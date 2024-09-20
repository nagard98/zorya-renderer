#define STB_IMAGE_IMPLEMENTATION  

#include "Texture2D.h"
#include "editor/Logger.h"

#include "stb_image.h"
#include "yaml.h"
#include <cinttypes>

namespace zorya
{
    Texture_Import_Config::Texture_Import_Config(const char* asset_file_path) : Asset_Import_Config(Asset_Type::TEXTURE, asset_file_path)
    {
        channels = 0;
        max_height = 0;
        max_width = 0;
        is_normal_map = false;
    }

    Texture_Import_Config* Texture_Import_Config::deserialize(const char* metafile_path)
    {
        Texture_Import_Config* tex_import = new Texture_Import_Config;

        yaml_parser_t parser;
        yaml_event_t event;

        yaml_parser_initialize(&parser);

        FILE* file = fopen(metafile_path, "rb");
        yaml_parser_set_input_file(&parser, file);

        bool is_stream_ended = false;

        do
        {
            int status = yaml_parser_parse(&parser, &event);
            if (status == 0)
            {
                fprintf(stderr, "yaml_parser_parse error\n");
                is_stream_ended = true;
            }

            switch (event.type)
            {
            case (YAML_STREAM_END_EVENT):
            {
                is_stream_ended = true;
                break;
            }

            case (YAML_SCALAR_EVENT):
            {
                if (strncmp((const char*)event.data.scalar.value, "asset_path", strnlen_s("asset_path", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    size_t filepath_len = strnlen((char*)event.data.scalar.value, MAX_PATH) + 1;
                    tex_import->asset_filepath = (char*)malloc(filepath_len);
                    if (status != 0) memcpy_s(tex_import->asset_filepath, filepath_len, event.data.scalar.value, filepath_len);
                } 
                else if (strncmp((const char*)event.data.scalar.value, "guid", strnlen_s("guid", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%" PRIu64, &tex_import->guid);
                } 
                else if (strncmp((const char*)event.data.scalar.value, "asset_type", strnlen_s("asset_type", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%d", &tex_import->asset_type);
                }
                
                if (strncmp((const char*)event.data.scalar.value, "channels", strnlen_s("channels", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%" PRIu8, &tex_import->channels);
                } 
                else if (strncmp((const char*)event.data.scalar.value, "width", strnlen_s("width", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%" PRIu32, &tex_import->max_width);
                } 
                else if (strncmp((const char*)event.data.scalar.value, "height", strnlen_s("height", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%" PRIu32, &tex_import->max_height);
                }
                else if (strncmp((const char*)event.data.scalar.value, "is_normal_map", strnlen_s("is_normal_map", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%d", &tex_import->is_normal_map);
                }
                break;
            }
            default:
            {
                break;
            }

            }

            yaml_event_delete(&event);

        } while (!is_stream_ended);

        fclose(file);
        yaml_parser_delete(&parser);

        return tex_import;
    }

    int Texture_Import_Config::serialize(Texture_Import_Config* tex_imp_conf)
    {
        yaml_emitter_t emitter;
        yaml_event_t event;

        yaml_emitter_initialize(&emitter);

        //size_t metafile_buff_size = strnlen_s(tex_imp_conf->asset_filepath, MAX_PATH) + strlen(".metafile") + 1;
        char metafile_path[MAX_PATH]; // = (char*)malloc(metafile_buff_size);
        ZeroMemory(metafile_path, MAX_PATH);
        sprintf(metafile_path, "%s.metafile", tex_imp_conf->asset_filepath);
        //strncat_s(metafile_path, metafile_buff_size, tex_imp_conf->asset_filepath, metafile_buff_size);
        //strncat_s(metafile_path, metafile_buff_size, ".metafile", strlen(".metafile"));

        FILE* file = fopen(metafile_path, "r");
        if (file == nullptr)
        {
            Texture2D::load_asset_info(tex_imp_conf);
        } else
        {
            fclose(file);
        }

        //TODO: check if failed file open
        file = fopen(metafile_path, "wb");

        yaml_emitter_set_output_file(&emitter, file);

        yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
        if (!yaml_emitter_emit(&emitter, &event)) goto error;

        yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0);
        if (!yaml_emitter_emit(&emitter, &event)) goto error;

        char tmp_string[100];

        {
            yaml_mapping_start_event_initialize(&event, NULL, (yaml_char_t*)YAML_MAP_TAG, 1, YAML_ANY_MAPPING_STYLE);
            if (!yaml_emitter_emit(&emitter, &event)) goto error;

            {
                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"guid", strlen("guid"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    sprintf(tmp_string, "%" PRIu64, tex_imp_conf->guid);
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_path", strlen("asset_path"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)tex_imp_conf->asset_filepath, strlen(tex_imp_conf->asset_filepath), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_type", strlen("asset_type"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    sprintf(tmp_string, "%d", tex_imp_conf->asset_type);
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"channels", strlen("channels"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    sprintf(tmp_string, "%d", tex_imp_conf->channels);
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"width", strlen("width"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    sprintf(tmp_string, "%d", tex_imp_conf->max_width);
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"height", strlen("height"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    sprintf(tmp_string, "%d", tex_imp_conf->max_width);
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"is_normal_map", strlen("is_normal_map"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    sprintf(tmp_string, "%d", tex_imp_conf->is_normal_map);
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }
            }

            yaml_mapping_end_event_initialize(&event);
            if (!yaml_emitter_emit(&emitter, &event)) goto error;

        }

        yaml_document_end_event_initialize(&event, 0);
        if (!yaml_emitter_emit(&emitter, &event)) goto error;

        yaml_stream_end_event_initialize(&event);
        if (!yaml_emitter_emit(&emitter, &event)) goto error;


        yaml_emitter_delete(&emitter);
        yaml_event_delete(&event);

        fclose(file);

        return 1;

    error:
        yaml_emitter_delete(&emitter);
        yaml_event_delete(&event);
        fclose(file);

        return 0;
    }

    Texture2D* Texture2D::create(const Texture_Import_Config* tex_imp_conf)
    {
        assert(tex_imp_conf != nullptr);

        //TODO: use specific allocator for texture asset; dont use global allocator
        Texture2D* texture = new Texture2D(tex_imp_conf);

        return texture;
    }

    Texture2D::Texture2D(const char* filepath) : Asset(filepath)
    {
        OutputDebugString("Currently this shouldn't be called");
        asset_type = Asset_Type::TEXTURE;

        m_data = nullptr;
    }

    Texture2D::Texture2D(const Texture_Import_Config* tex_import_config) : Asset(tex_import_config)
    {
        m_data = nullptr;
    }

    void Texture2D::load_asset(const Asset_Import_Config* asset_import_conf)
    {
        assert(asset_import_conf->asset_type == Asset_Type::TEXTURE);
        const Texture_Import_Config* tex_import_conf = static_cast<const Texture_Import_Config*>(asset_import_conf);

        if (!m_is_loaded)
        {
            int x, y, components;

            //TODO: add functionality to import texture with specified configuration
            m_data = stbi_load(m_file_path.c_str(), &x, &y, &components, 4);
            const char* fail_reason = stbi_failure_reason();
            if (m_data == nullptr)
            {
                Logger::add_log(Logger::Channel::ERR, "Failed to import %s image data :: %s", m_file_path.c_str(), fail_reason);
            } 
            else
            {
                if ((x < UINT32_MAX) && (y < UINT32_MAX))
                {
                    m_is_loaded = true;
                } else
                {
                    stbi_image_free(m_data);
                    m_data = nullptr;
                }
            }
        }
    }

    int Texture2D::load_asset_info(Texture_Import_Config* tex_import_conf)
    {
        int result = stbi_info(
            tex_import_conf->asset_filepath, 
            (int*)&tex_import_conf->max_width, 
            (int*)&tex_import_conf->max_height, 
            (int*)&tex_import_conf->channels
        );

        return result;
    }

    void Texture2D::update_asset(const Texture_Import_Config* tex_imp_config)
    {
        assert(tex_imp_config->asset_type == Asset_Type::TEXTURE);
        const Texture_Import_Config* tex_import_conf = static_cast<const Texture_Import_Config*>(tex_imp_config);

        if (m_is_loaded)
        {
            assert(m_data != nullptr);
            stbi_image_free(m_data);
            m_data = nullptr;

            int x, y, components;

            //TODO: add functionality to import texture with specified configuration
            m_data = stbi_load(m_file_path.c_str(), &x, &y, &components, 4);
            //const char* err = stbi_failure_reason();

            if ((x < UINT32_MAX) && (y < UINT32_MAX))
            {
                //m_width = static_cast<uint32_t>(x);
                //m_height = static_cast<uint32_t>(y);
                //m_channels = static_cast<uint8_t>(4);
                m_is_loaded = true;
            } else
            {
                stbi_image_free(m_data);
                m_data = nullptr;
            }
        }
    }

    //int Texture2D::serialize()
    //{
    //    yaml_emitter_t emitter;
    //    yaml_event_t event;

    //    yaml_emitter_initialize(&emitter);

    //    size_t metafile_buff_size = strnlen_s(m_file_path.c_str(), MAX_PATH) + strlen(".metafile") + 1;
    //    char* metafile_path = (char*)malloc(metafile_buff_size);
    //    metafile_path[0] = 0;
    //    strncat_s(metafile_path, metafile_buff_size, m_file_path.c_str(), metafile_buff_size);
    //    strncat_s(metafile_path, metafile_buff_size, ".metafile", strlen(".metafile"));

    //    FILE* file = fopen(metafile_path, "wb");

    //    yaml_emitter_set_output_file(&emitter, file);

    //    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    //    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //    yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0);
    //    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //    char tmp_string[100];

    //    {
    //        yaml_mapping_start_event_initialize(&event, NULL, (yaml_char_t*)YAML_MAP_TAG, 1, YAML_ANY_MAPPING_STYLE);
    //        if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //        {
    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"guid", strlen("guid"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%" PRIu64, m_guid);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_path", strlen("asset_path"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)m_file_path.c_str(), strlen(m_file_path.c_str()), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_type", strlen("asset_type"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", asset_type);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"channels", strlen("channels"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", m_channels);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"width", strlen("width"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", m_width);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"height", strlen("height"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", m_height);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //        }

    //        yaml_mapping_end_event_initialize(&event);
    //        if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //    }

    //    yaml_document_end_event_initialize(&event, 0);
    //    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //    yaml_stream_end_event_initialize(&event);
    //    if (!yaml_emitter_emit(&emitter, &event)) goto error;


    //    yaml_emitter_delete(&emitter);
    //    yaml_event_delete(&event);
    //    free(metafile_path);

    //    fclose(file);

    //    return 1;

    //error:
    //    free(metafile_path);
    //    yaml_emitter_delete(&emitter);
    //    yaml_event_delete(&event);
    //    fclose(file);

    //    return 0;
    //}
}