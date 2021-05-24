#include "daqi/templates.hpp"

#include "inja/inja.hpp"

#include "daqi/def/log_def.hpp"
#include "daqi/def/boost_def.hpp"
#include "daqi/utilities/string_utilities.hpp"
#include "daqi/utilities/file_utilities.hpp"

namespace da4qi4
{

std::string const daqi_HTML_template_ext = ".daqi.HTML";
std::string const daqi_JSON_template_ext = ".daqi.JSON";
std::string const daqi_XML_template_ext = ".daqi.XML";

std::string const& get_daqi_HTML_template_ext()
{
    return daqi_HTML_template_ext;
}

std::string const& get_daqi_JSON_template_ext()
{
    return daqi_JSON_template_ext;
}

std::string const& get_daqi_XML_template_ext()
{
    return daqi_XML_template_ext;
}

std::string make_daqi_template_ext(std::string const& ext)
{
    return std::string(".daqi") + (ext.empty() ? "" : (ext[0] == '.' ? ext : ("." + ext)));
}

void init_template_env(inja::Environment& env)
{
    env.set_expression("{=", "=}");

    env.set_comment("{#", "#}");
    env.set_statement("{%", "%}");
    env.set_line_statement("##");

    env.set_element_notation(inja::ElementNotation::Dot);

    env.set_trim_blocks(true);
    env.set_lstrip_blocks(true);
}

std::string remove_template_ext(std::string const& fn, std::string const& ext)
{
    return (Utilities::EndsWith(fn, ext)) ?
           fn.substr(0, fn.length() - ext.length()) : fn;
}

bool is_include_dir(fs::path const& path)
{
    return (path.native().find("/i/") != std::string::npos
            || path.native().find("\\i\\") != std::string::npos);
}

bool Templates::try_load_template(TemplatesEnv& env
                                  , std::string const& key
                                  , std::string const& template_filename
                                  , std::string const& full_template_filename
                                  , bool is_include_dir) noexcept
{
    try
    {
        std::string tmpl_src = env.load_file(template_filename);

        if (tmpl_src.empty())
        {
            _app_logger->error("Template read fail. \"{}\".", full_template_filename);
            return false;
        }

        TemplatePtr templ = TemplatePtr(new Template(env.parse(tmpl_src)));
        Item item {templ, full_template_filename};

        if (is_include_dir)
        {
            env.include_template(key, *templ);
            _includes_templates.insert(std::make_pair(key, std::move(item)));
        }
        else
        {
            _templates.insert(std::make_pair(key, std::move(item)));
        }

        _app_logger->info("Template load success. \"{}\". {}.",
                          remove_template_ext(template_filename, _template_ext), key);
        return true;
    }
    catch (std::exception const& e)
    {
        _app_logger->error("Template load exception. {}. \"{}\". {}.", e.what()
                           , full_template_filename, key);
        return false;
    }
    catch (...)
    {
        _app_logger->error("Template load exception. \"{}\". {}.", full_template_filename, key);
        return false;
    }
}

bool is_ignored(fs::path const& path)
{
    std::string const& path_s = path.string();

    return (path.empty()
            || path_s.find("/_alternate/") != std::string::npos
            || path_s.find("/_deprecated/") != std::string::npos
            || path_s.find(".deprecated.") != std::string::npos);
}

std::pair<size_t, size_t> Templates::load_templates(std::string const& template_ext
                                                    , std::string const& key_ext)
{
    TemplatesEnv env(_root);
    init_template_env(env);

    std::pair<size_t, size_t> counts_i = load_templates(env, template_ext, key_ext
                                                        , TemplateFlag::for_include);

    std::pair<size_t, size_t> counts = load_templates(env, template_ext, key_ext
                                                      , TemplateFlag::for_normal);

    return { counts_i.first + counts.first, counts_i.second + counts.second };
}

std::string make_template_key(std::string const& app_prefix, std::string const& mpath
                              , std::string const& key_ext)
{
    std::string key;
    key.reserve(app_prefix.size() + mpath.size() + key_ext.size() + 1);

    key = app_prefix;

    if (app_prefix.empty() || mpath.empty()
        || *(--app_prefix.end()) != '/' || *(mpath.begin()) != '/')
    {
        key += mpath;
    }
    else
    {
        key += mpath.substr(1);
    }

    key += key_ext;

    return key;
}

std::pair<size_t, size_t> Templates::load_templates(TemplatesEnv& env,
                                                    std::string const& template_ext,
                                                    std::string const& key_ext,
                                                    TemplateFlag flag)
{
    fs::path root(_root);

    if (!fs::is_directory((root)) || !fs::exists(root))
    {
        return {0, 0};
    }

    size_t ok = 0, fail = 0;

    fs::recursive_directory_iterator iter(root);
    fs::recursive_directory_iterator end_iter;

    for (; iter != end_iter; ++iter)
    {
        if (fs::is_regular_file(*iter))
        {
            fs::path const& path = *iter;

            if (Utilities::EndsWith(path.filename().native(), template_ext))
            {
                if (is_ignored(path))
                {
                    continue;
                }

                bool is_include = is_include_dir(path.string());

                if ((flag == TemplateFlag::for_normal && is_include)
                    || (flag == TemplateFlag::for_include && !is_include))
                {
                    continue;
                }

                std::string::size_type len = path.size();
                std::string::size_type root_len = root.size();
                std::string mpath = path.string()./*native().*/
                                    substr(root_len, len - root_len - template_ext.size());

                if (!mpath.empty())
                {
                    std::string template_filename = mpath + template_ext;
                    std::string key = make_template_key(_app_prefix, mpath, key_ext);

                    if (try_load_template(env, key, template_filename, path.string(), is_include))
                    {
                        ++ok;
                    }
                    else
                    {
                        ++fail;
                    }
                }
            }
        }
    }

    return {ok, fail};
}

bool Templates::reload()
{
    return (!_app_logger) ? false : Preload(_app_logger);
}

void Templates::CopyIncludeTemplateTo(TemplatesEnv& env)
{
    for (auto pair : _includes_templates)
    {
        env.include_template(pair.first, *(pair.second.templ));
    }
}

bool Templates::Preload(log::LoggerPtr app_logger)
{
    if (_app_logger != app_logger)
    {
        _app_logger = app_logger;
    }

    if (this->_disabled || this->_root.empty() || this->_template_ext.empty())
    {
        _app_logger->info("Templates is undesired.");
        return true;
    }

    std::lock_guard<std::mutex> _guard_(_m);

    _templates.clear();
    _includes_templates.clear();

    try
    {
        auto counts = load_templates(_template_ext, Utilities::theEmptyString);
        _loaded_time = std::time(nullptr);
        app_logger->info("Templates loaded. {} success, {} fail.", counts.first, counts.second);

        return true;
    }
    catch (fs::filesystem_error const& ec)
    {
        _app_logger->error("Template file \"{}\" load exception.", ec.what());
    }
    catch (std::exception const& ec)
    {
        _app_logger->error("Template file \"{}\" load exception.", ec.what());
    }
    catch (...)
    {
        _app_logger->error("Template file load unknown exception.");
    }

    return false;
}

TemplatePtr const Templates::Get(std::string const& name)
{
    std::lock_guard<std::mutex> _guard_(_m);

    auto it = _templates.find(name);

    if (it == _templates.end())
    {
        return nullptr;
    }

    return it->second.templ;
}

void Templates::hint_template_updated_found(TemplateUpdateAction action)
{
    switch (action)
    {
        case TemplateUpdateAction::appended:
            _app_logger->info("Detects template appended.");
            break;

        case TemplateUpdateAction::modified:
            _app_logger->info("Detects template modified.");
            break;

        case TemplateUpdateAction::removed:
            _app_logger->info("Detects template removed.");
            break;

        default:
            break;
    }
}

void Templates::hint_template_reload_fail()
{
    _app_logger->error("Templates reload fail.");
}

bool Templates::ReloadIfFindUpdate()
{
    if (this->_disabled)
    {
        return true;
    }

    auto r = check_exists_template();

    if (r == TemplateUpdateAction::none)
    {
        return false;
    }

    hint_template_updated_found(r);

    if (reload())
    {
        return true;
    }

    hint_template_reload_fail();
    return false;
}

Templates::TemplateUpdateAction
Templates::check_exists_template(std::unordered_map<std::string, Item> const& templates)
{
    TemplateUpdateAction action = TemplateUpdateAction::none;

    for (auto item : templates)
    {
        auto fp = fs::path(item.second.filename);

        if (!Utilities::IsFileExists(fp))
        {
            action = TemplateUpdateAction::removed;
            break;
        }

        errorcode ec;
        std::time_t t = fs::last_write_time(fp, ec);

        if (ec)
        {
            _app_logger->warn("Check templates file \"{}\" last-write-time exception. {}",
                              item.second.filename, ec.message());
        }
        else if (t > _loaded_time)
        {
            action = TemplateUpdateAction::modified;
            break;
        }
    }

    return action;
}

Templates::TemplateUpdateAction Templates::check_exists_template()
{
    TemplateUpdateAction action = check_exists_template(_templates);

    if (action != TemplateUpdateAction::none)
    {
        return action;
    }

    return check_exists_template(_includes_templates);
}

bool Templates::ReloadIfFindNew()
{
    if (this->_disabled)
    {
        return true;
    }

    auto r = check_new_template(_template_ext, Utilities::theEmptyString);

    if (r == TemplateUpdateAction::none)
    {
        return false;
    }

    hint_template_updated_found(r);

    if (reload())
    {
        return true;
    }

    hint_template_reload_fail();
    return false;
}

Templates::TemplateUpdateAction Templates::check_new_template(std::string const& template_ext,
                                                              std::string const& key_ext)
{
    fs::path root(_root);

    if (!fs::is_directory((root)) || !fs::exists(root))
    {
        return TemplateUpdateAction::none;
    }

    fs::recursive_directory_iterator iter(root);
    fs::recursive_directory_iterator end_iter;

    for (; iter != end_iter; ++iter)
    {
        if (fs::is_regular_file(*iter))
        {
            fs::path const& path = *iter;

            if (Utilities::EndsWith(path.filename().native(), template_ext))
            {
                if (is_ignored(path))
                {
                    continue;
                }

                std::string::size_type len = path.size();
                std::string::size_type root_len = root.size();
                std::string mpath = path.native().substr(root_len
                                                         , len - root_len - template_ext.size());

                if (!mpath.empty())
                {
                    bool is_include = is_include_dir(path);
                    std::string key = make_template_key(_app_prefix, mpath, key_ext);

                    std::lock_guard<std::mutex> _guard_(_m);

                    if ((!is_include && (_templates.find(key) == _templates.cend()))
                        || (is_include && (_includes_templates.find(key)
                                           == _includes_templates.cend())))
                    {
                        return TemplateUpdateAction::appended;
                    }
                }
            }
        }
    }

    return TemplateUpdateAction::none;
}


} //namespace da4qi4
