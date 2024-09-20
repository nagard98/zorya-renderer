#include "Material.h"
#include "Shader.h"
#include "renderer/backend/ResourceCache.h"
#include "editor/AssetRegistry.h"

#include <vector>
#include <cinttypes>

#include <yaml.h>

namespace zorya
{
    //Material2::Material2(Asset_Import_Config* import_config) : Asset(import_config)
    //{
    //    //TODO: fix everything; allocating material in different place and then copying; quick and dirty for testing
    //    material_hnd = { 0, IS_FIRST_MAT_ALLOC };
    //    material_hnd = resource_cache.alloc_material(material_hnd);
    //    Material2& mat = resource_cache.m_material_cache.at(material_hnd.index);

    //    resources = mat.resources;
    //    flags = mat.flags;
    //    shader = mat.shader;
    //    
    //}

    static int yaml_scalar_float_vec_event_initialize_and_emit(yaml_emitter_t *emitter, yaml_event_t* event, float* data, int num_components)
    {
        char tmp_string[100];
        char component_name[] = { 'r','g','b','a' };
        int err = 0;

        yaml_mapping_start_event_initialize(event, NULL, (yaml_char_t*)YAML_MAP_TAG, 1, YAML_FLOW_MAPPING_STYLE);
        err = yaml_emitter_emit(emitter, event);
        if (!err) return err;
        
        for (int i = 0; i < num_components; i++)
        {
            yaml_scalar_event_initialize(event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)&component_name[i], 1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
            err = yaml_emitter_emit(emitter, event);
            if (!err) return err;

            sprintf(tmp_string, "%f", *(data + i));
            yaml_scalar_event_initialize(event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
            err = yaml_emitter_emit(emitter, event);
            if (!err) return err;
        }

        yaml_mapping_end_event_initialize(event);
        err = yaml_emitter_emit(emitter, event);
        if (!err) return err;

        return err;
    }

    Material* Material::create(const Asset_Import_Config* import_config)
    {
        Material_Cache_Handle_t hnd_mat{ 0, IS_FIRST_MAT_ALLOC };
        hnd_mat = resource_cache.alloc_material(hnd_mat);
        Material* mat = &resource_cache.m_material_cache.at(hnd_mat.index);

        mat->m_guid = import_config->guid;
        mat->m_file_path = std::string(import_config->asset_filepath);
        mat->asset_type = import_config->asset_type;
        mat->m_is_loaded = false;

        return mat;
    }

    void Material::load_asset(const Asset_Import_Config* asset_imp_conf)
    {
        if (!m_is_loaded)
        {
            deserialize();
            m_is_loaded = true;
        }
    }

    //int serialize(Texture_Import_Config* tex_import_config)
    //{
    //    yaml_emitter_t emitter;
    //    yaml_event_t event;

    //    yaml_emitter_initialize(&emitter);

    //    size_t metafile_buff_size = strnlen_s(tex_import_config->asset_filepath, MAX_PATH) + strlen(".metafile") + 1;
    //    char* metafile_path = (char*)malloc(metafile_buff_size);
    //    metafile_path[0] = 0;
    //    strncat_s(metafile_path, metafile_buff_size, tex_import_config->asset_filepath, metafile_buff_size);
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

    //                sprintf(tmp_string, "%" PRIu64, tex_import_config->guid);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_path", strlen("asset_path"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)tex_import_config->asset_filepath, strlen(tex_import_config->asset_filepath), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"asset_type", strlen("asset_type"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", tex_import_config->asset_type);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"channels", strlen("channels"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", tex_import_config->channels);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"width", strlen("width"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", tex_import_config->max_width);
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;
    //            }

    //            {
    //                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"height", strlen("height"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
    //                if (!yaml_emitter_emit(&emitter, &event)) goto error;

    //                sprintf(tmp_string, "%d", tex_import_config->max_height);
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

    int Material::serialize()
	{
        yaml_emitter_t emitter;
        yaml_event_t event;

        yaml_emitter_initialize(&emitter);

        size_t metafile_buff_size = strnlen_s(m_file_path.c_str(), MAX_PATH) + strlen(".metafile") + 1;
        char* metafile_path = (char*)malloc(metafile_buff_size);
        metafile_path[0] = 0;
        strncat_s(metafile_path, metafile_buff_size, m_file_path.c_str(), metafile_buff_size);
        //strncat_s(metafile_path, metafile_buff_size, ".metafile", strlen(".metafile"));

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
                std::vector<Shader_Resource*> textures;
                std::vector<Shader_Resource*> parameter_buffers;

                for (auto& resource : resources)
                {
                    switch (resource.m_type)
                    {
                    case zorya::ZRY_TEX:
                    {
                        textures.push_back(&resource);
                        break;
                    }
                    case zorya::ZRY_CBUFF:
                    {
                        parameter_buffers.push_back(&resource);
                        break;
                    }
                    case zorya::ZRY_UNSUPPORTED:
                        break;
                    default:
                        break;
                    }
                }


                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"Textures", strnlen_s("Textures", MAX_PATH), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                if (!yaml_emitter_emit(&emitter, &event)) goto error;

                {
                    yaml_mapping_start_event_initialize(&event, NULL, (yaml_char_t*)YAML_MAP_TAG, 1, YAML_ANY_MAPPING_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    for (auto texture : textures)
                    {
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)texture->m_name, strlen(texture->m_name), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                        sprintf(tmp_string, "%" PRIu64, (texture->m_texture != nullptr) ? texture->m_texture->m_guid : 0);
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;
                    }

                    yaml_mapping_end_event_initialize(&event);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;
                }


                for (auto parameter_buffer : parameter_buffers)
                {
                    yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)parameter_buffer->m_name, strnlen_s(parameter_buffer->m_name, MAX_PATH), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    {
                        yaml_mapping_start_event_initialize(&event, NULL, (yaml_char_t*)YAML_MAP_TAG, 1, YAML_BLOCK_MAPPING_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                        for (int i = 0; i < parameter_buffer->m_parameters->num_variables; i++)
                        {
                            unsigned char* parameter_buffer_start = static_cast<unsigned char*>(parameter_buffer->m_parameters->cb_start);
                            CB_Variable& variable = parameter_buffer->m_parameters->variables[i];

                            if (variable.description.variable_type == VAR_REFL_TYPE::FLOAT)
                            {
                                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)variable.name, strlen(variable.name), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                                if (!yaml_emitter_emit(&emitter, &event)) goto error;

                                int err = yaml_scalar_float_vec_event_initialize_and_emit(&emitter, &event, (float*)(parameter_buffer_start + variable.offset_in_bytes), variable.description.columns);
                                if (!err) goto error;
                            }
                        }


                        yaml_mapping_end_event_initialize(&event);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    }
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

    int Material::deserialize()
    {
        yaml_parser_t parser;
        yaml_event_t event;

        yaml_parser_initialize(&parser);

        FILE* file = fopen(m_file_path.c_str(), "rb");
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
            case (YAML_MAPPING_START_EVENT):
            {
                status = yaml_parser_parse(&parser, &event);
                assert(status != 0);
                if (event.type == YAML_SCALAR_EVENT)
                {
                    if (strncmp((char*)event.data.scalar.value, "Textures", strnlen("Textures", MAX_PATH)) == 0)
                    {
                        status = yaml_parser_parse(&parser, &event); // MAPPING_START

                        status = yaml_parser_parse(&parser, &event); // FIRST SCALAR
                        do
                        {
                            char tex_name[MAX_PATH] = {};
                            sprintf(tex_name, "%s", (char*)event.data.scalar.value);

                            status = yaml_parser_parse(&parser, &event);
                            uint64_t guid = 0;
                            sscanf((char*)event.data.scalar.value, "%" PRIu64, &guid);
                            Asset_With_Config* asset_with_conf = asset_registry.get_asset_by_guid(guid);

                            if (asset_with_conf != nullptr)
                            {
                                set_shader_resource_asset(resources, tex_name, 
                                    static_cast<Texture2D*>(asset_with_conf->asset), 
                                    static_cast<Texture_Import_Config*>(asset_with_conf->import_config)
                                );
                            }
                            else
                            {

                                set_shader_resource_asset(resources, tex_name, nullptr, nullptr);
                            }

                            status = yaml_parser_parse(&parser, &event); // EITHER KEY SCALAR OR MAPPING END

                        } while (event.type != YAML_MAPPING_END_EVENT);
                    } 
                    
                    status = yaml_parser_parse(&parser, &event);

                    if (strncmp((char*)event.data.scalar.value, "_matPrms", strnlen("_matPrms", MAX_PATH)) == 0)
                    {
                        Constant_Buffer_Data* cb_data = nullptr;
                        for (auto& resource : resources)
                        {
                            if (resource.m_type == Resource_Type::ZRY_CBUFF) cb_data = resource.m_parameters;
                        }

                        status = yaml_parser_parse(&parser, &event); // MAPPING_START

                        status = yaml_parser_parse(&parser, &event); // FIRST SCALAR

                        int num_components = 0;
                        float param_data[4];
                        do
                        {
                            char param_name[MAX_PATH] = {};
                            sprintf(param_name, "%s", (char*)event.data.scalar.value);

                            status = yaml_parser_parse(&parser, &event);
                            if (!status) return status;

                            status = yaml_parser_parse(&parser, &event);
                            if (!status) return status;

                            do
                            {
                                status = yaml_parser_parse(&parser, &event);
                                if (!status) return status;

                                //TODO: here assumes all parameters are float vectors
                                sscanf((char*)event.data.scalar.value, "%f", &param_data[num_components]);

                                num_components += 1;

                                status = yaml_parser_parse(&parser, &event);
                                if (!status) return status;
                            } while (event.type != YAML_MAPPING_END_EVENT);

                            bool is_var_set = set_constant_buff_var(cb_data, param_name, param_data, num_components * sizeof(float));
                            assert(is_var_set == true);

                            status = yaml_parser_parse(&parser, &event); // EITHER KEY SCALAR OR MAPPING END
                            num_components = 0;

                        } while (event.type != YAML_MAPPING_END_EVENT);
                    }
                }

                break;
            }
            case (YAML_SCALAR_EVENT):
            default:
            {
                break;
            }

            }

        } while (!is_stream_ended);

        m_is_loaded = true;
        update_standard_internals(*this);

        fclose(file);

        yaml_event_delete(&event);
        yaml_parser_delete(&parser);
        
        //TODO: return significant value
        return 0;
    }


	std::vector<Shader_Resource*> Material::get_material_editor_resources()
	{
		int max_num_resources = resources.size();
		
		std::vector<Shader_Resource*> material_resources;
		material_resources.reserve(max_num_resources);
		for (int i = 0; i < max_num_resources; i++)
		{
			if (resources[i].m_name[0] == '_') material_resources.push_back(&resources[i]);
		}

		material_resources.shrink_to_fit();

		return material_resources;
	}
	
	void update_standard_internals(Material& material)
	{
		Constant_Buffer_Data* cb_data = nullptr;

		for (auto& resource : material.resources)
		{
			if (strncmp(resource.m_name, "_matPrms", strnlen_s("_matPrms", MAX_PATH)) == 0) cb_data = resource.m_parameters;
		}

		if (cb_data != nullptr)
		{
			for (auto& resource : material.resources)
			{
				if (strncmp(resource.m_name, "_ObjTexture", strnlen_s("_ObjTexture", MAX_PATH)) == 0)
				{
					//bool val = resource.m_fake_hnd_resource != nullptr;
					bool val = resource.m_hnd_gpu_srv.index != 0;
					bool suc = set_constant_buff_var(cb_data, "hasAlbedoMap", &val, sizeof(val));
					assert(suc);
					continue;
				}
				
				if (strncmp(resource.m_name, "_NormalMap", strnlen_s("_NormalMap", MAX_PATH)) == 0)
				{
					//bool val = resource.m_fake_hnd_resource != nullptr;
					bool val = resource.m_hnd_gpu_srv.index != 0;
					bool suc = set_constant_buff_var(cb_data, "hasNormalMap", &val, sizeof(val));
					assert(suc);
					continue;
				}

				if (strncmp(resource.m_name, "_MetalnessMap", strnlen_s("_MetalnessMap", MAX_PATH)) == 0)
				{
					//bool val = resource.m_fake_hnd_resource != nullptr;
					bool val = resource.m_hnd_gpu_srv.index != 0;
					bool suc = set_constant_buff_var(cb_data, "hasMetalnessMap", &val, sizeof(val));
					assert(suc);
					continue;
				}

				if (strncmp(resource.m_name, "_SmoothnessMap", strnlen_s("_SmoothnessMap", MAX_PATH)) == 0)
				{
					bool val = resource.m_hnd_gpu_srv.index != 0 ;
					bool suc = set_constant_buff_var(cb_data, "hasSmoothnessMap", &val, sizeof(val));
					assert(suc);
					continue;
				}
			}
		}

	}
}


