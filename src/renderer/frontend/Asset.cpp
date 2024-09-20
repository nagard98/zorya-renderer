#include "Asset.h"
#include "Texture2D.h"
#include "Model.h"
#include "Material.h"
#include "DiffusionProfile.h"

#include <stb_image.h>
#include <yaml.h>

#include <cassert>
#include <cinttypes>
#include <Shlwapi.h>
#include <combaseapi.h>

#pragma comment(lib, "Shlwapi.lib")

namespace zorya
{
    Asset::Asset()
    {
        GUID guid;
        assert(CoCreateGuid(&guid) == S_OK);
        memcpy(&m_guid, &guid, sizeof(m_guid));

        m_file_path = std::string();
        m_is_loaded = false;
    }

    Asset::Asset(const char* filepath)
    {
        GUID guid;
        assert(CoCreateGuid(&guid) == S_OK);
        memcpy(&m_guid, &guid, sizeof(m_guid));

        m_file_path = std::string(filepath);
        m_is_loaded = false;
    }

    Asset::Asset(const Asset_Import_Config* asset_import_config)
    {
        m_guid = asset_import_config->guid;
        m_file_path = std::string(asset_import_config->asset_filepath);
        asset_type = asset_import_config->asset_type;
        m_is_loaded = false;
    }

    bool is_supported_img_extension(const char* extension, size_t extension_len)
    {
        return (strncmp(extension, ".png", extension_len) == 0) ||
            (strncmp(extension, ".jpg", extension_len) == 0) ||
            (strncmp(extension, ".jpeg", extension_len) == 0) ||
            (strncmp(extension, ".tga", extension_len) == 0);
    }

    bool is_supported_text_extension(const char* extension, size_t extension_len)
    {
        return (strncmp(extension, ".txt", extension_len) == 0);
    }

    bool is_supported_model_extension(const char* extension, size_t extension_len)
    {
        return (strncmp(extension, ".obj", extension_len) == 0) ||
            (strncmp(extension, ".fbx", extension_len) == 0) ||
            (strncmp(extension, ".gltf", extension_len) == 0);
    }

    bool is_supported_material_extension(const char* extension, size_t extension_len)
    {
        return (strncmp(extension, ".mat", extension_len) == 0);
    }

    Asset_Type get_asset_type_by_ext(const char* extension)
    {
        if (is_supported_img_extension(extension, 5)) return Asset_Type::TEXTURE;
        else if (is_supported_model_extension(extension, 5)) return Asset_Type::STATIC_MESH;
        else if (is_supported_material_extension(extension, 5)) return Asset_Type::MATERIAL;
        else return Asset_Type::UNSUPPORTED;
    }

    size_t get_filename_length(const char* filepath)
    {
        HANDLE hnd_file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        size_t buff_size = MAX_PATH * sizeof(WCHAR) + sizeof(FILE_NAME_INFO);
        char* buff_file_name_info = (char*)malloc(buff_size);

        FILE_NAME_INFO* file_name_info = reinterpret_cast<FILE_NAME_INFO*>(buff_file_name_info);

        size_t filename_length = 0;

        if (GetFileInformationByHandleEx(hnd_file, _FILE_INFO_BY_HANDLE_CLASS::FileNameInfo, static_cast<void*>(file_name_info), buff_size))
        {
            filename_length = file_name_info->FileNameLength;
        }

        free(buff_file_name_info);
        CloseHandle(hnd_file);

        return filename_length;
    }



    Asset* create_asset(const Asset_Import_Config* asset_imp_config)
    {
        Asset* asset = nullptr;

        //TODO: think about how to and where to allocate these resources
        switch (asset_imp_config->asset_type)
        {
        case zorya::TEXTURE:
        {
            asset = Texture2D::create(static_cast<const Texture_Import_Config*>(asset_imp_config));
            break;
        }
        case zorya::STATIC_MESH:
        {
            asset = Model3D::create(asset_imp_config);
            break;
        }
        case zorya::MATERIAL:
        {
            asset = Material::create(asset_imp_config);
            break;
        }
        case zorya::DIFFUSION_PROFILE:
        {
            asset = Diffusion_Profile::create(asset_imp_config);
            break;
        }
        default:
            break;
        }

        //size_t filename_length = get_filename_length(filepath);

        //char* extension_delimiter = PathFindExtension(filepath);
        //size_t extension_len = strnlen_s(extension_delimiter, MAX_PATH);

        //if ((extension_len <= filename_length) && (extension_len > 0))
        //{
        //    if (strncmp(".metafile", extension_delimiter, strnlen_s(".metafile", MAX_PATH)) == 0)
        //    {
        //        Asset_Import_Config* imp_config = nullptr;//deserialize(filepath);

        //        //TODO: think about how to and where to allocate these resources
        //        //switch (imp_config->asset_type)
        //        //{
        //        //case zorya::TEXTURE:
        //        //{
        //        //    asset = new Texture2D(static_cast<Texture_Import_Config*>(imp_config));
        //        //    break;
        //        //}
        //        //case zorya::STATIC_MESH:
        //        //{
        //        //    break;
        //        //}
        //        //case zorya::MATERIAL:
        //        //{
        //        //    asset = Material2::create_material(imp_config);
        //        //    break;
        //        //}
        //        //default:
        //        //    break;
        //        //}
        //    }    
        //}

        return asset;
    }

    Asset_Import_Config::Asset_Import_Config(Asset_Type type, const char* asset_file_path)
    {
        GUID win32_guid;
        assert(CoCreateGuid(&win32_guid) == S_OK);
        memcpy(&guid, &win32_guid, sizeof(guid));

        asset_type = type;
        
        size_t path_buff_size = strnlen_s(asset_file_path, MAX_PATH) + 1;
        asset_filepath = (char*)malloc(path_buff_size);
        strncpy_s(asset_filepath, path_buff_size, asset_file_path, path_buff_size);
    }

    Asset_Import_Config* Asset_Import_Config::deserialize(const char* metafile_path)
    {
        //TODO: add first line of metafile containing type of asset; this way you dont need generic Asset_Import_Config
        // before building specific import config
        Asset_Import_Config* asset_imp_conf = new Asset_Import_Config;

        yaml_parser_t parser;
        yaml_event_t event;

        yaml_parser_initialize(&parser);

        //TODO: add check if file failed to open
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
                    asset_imp_conf->asset_filepath = (char*)malloc(filepath_len);
                    if (status != 0) memcpy_s(asset_imp_conf->asset_filepath, filepath_len, event.data.scalar.value, filepath_len);
                } 
                else if (strncmp((const char*)event.data.scalar.value, "guid", strnlen_s("guid", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%" PRIu64, &asset_imp_conf->guid);
                } 
                else if (strncmp((const char*)event.data.scalar.value, "asset_type", strnlen_s("asset_type", MAX_PATH)) == 0)
                {
                    status = yaml_parser_parse(&parser, &event);
                    if (status != 0) sscanf_s((const char*)event.data.scalar.value, "%d", &asset_imp_conf->asset_type);
                }

                break;
            }

            default:
                break;

            }

        } while (!is_stream_ended);

        switch (asset_imp_conf->asset_type)
        {
        case zorya::TEXTURE:
        {
            delete asset_imp_conf;
            asset_imp_conf = Texture_Import_Config::deserialize(metafile_path);
            break;
        }
        case zorya::STATIC_MESH:
        case zorya::MATERIAL:
        case zorya::DIFFUSION_PROFILE:
        default:
            break;
        }

        fclose(file);
        yaml_parser_delete(&parser);

        return asset_imp_conf;
    }

    int Asset_Import_Config::serialize(const Asset_Import_Config* asset_imp_config)
    {
        yaml_emitter_t emitter;
        yaml_event_t event;

        yaml_emitter_initialize(&emitter);

        size_t metafile_buff_size = strnlen_s(asset_imp_config->asset_filepath, MAX_PATH) + strlen(".metafile") + 1;
        char* metafile_path = (char*)malloc(metafile_buff_size);
        metafile_path[0] = 0;
        strncat_s(metafile_path, metafile_buff_size, asset_imp_config->asset_filepath, metafile_buff_size);
        strncat_s(metafile_path, metafile_buff_size, ".metafile", strlen(".metafile"));

        //TODO: check if failed file open
        FILE* file = fopen(metafile_path, "wb");

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

                    sprintf(tmp_string, "%" PRIu64, asset_imp_config->guid);
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_path", strlen("asset_path"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)asset_imp_config->asset_filepath, strlen(asset_imp_config->asset_filepath), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }

                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_type", strlen("asset_type"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    sprintf(tmp_string, "%d", asset_imp_config->asset_type);
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
        free(metafile_path);

        fclose(file);

        return 1;

    error:
        free(metafile_path);
        yaml_emitter_delete(&emitter);
        yaml_event_delete(&event);
        fclose(file);

        return 0;
    }

    Asset_Import_Config* create_asset_import_config(const char* metafile_path)
    {
        Asset_Import_Config* asset_imp_conf = nullptr;
        size_t filename_length = get_filename_length(metafile_path);

        char* extension_delimiter = PathFindExtension(metafile_path);
        size_t extension_len = strnlen_s(extension_delimiter, MAX_PATH);

        if ((extension_len <= filename_length) && (extension_len > 0))
        {
            if (strncmp(".metafile", extension_delimiter, strnlen_s(".metafile", MAX_PATH)) == 0)
            {
                asset_imp_conf = Asset_Import_Config::deserialize(metafile_path);
            }
        }

        return asset_imp_conf;
    }

    Asset_Import_Config* create_asset_import_config(Asset_Type asset_type, const char* referenced_asset_file_path)
    {
        Asset_Import_Config* asset_imp_conf = nullptr;
        
        switch (asset_type)
        {
        
        case zorya::TEXTURE:
        {
            asset_imp_conf = new Texture_Import_Config(referenced_asset_file_path);
            Texture_Import_Config::serialize(static_cast<Texture_Import_Config*>(asset_imp_conf));
            break;
        }

        case zorya::STATIC_MESH:
        case zorya::MATERIAL:
        case zorya::DIFFUSION_PROFILE:
        {
            asset_imp_conf = new Asset_Import_Config(asset_type, referenced_asset_file_path);
            Asset_Import_Config::serialize(asset_imp_conf);
            break;
        }
        default:
            break;
        }

        return asset_imp_conf;
    }
    
}
