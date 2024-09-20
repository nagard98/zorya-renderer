#include "DiffusionProfile.h"
#include "editor/AssetRegistry.h"
#include "renderer/backend/ResourceCache.h"

#include "yaml.h"
#include <cinttypes>

namespace zorya
{

	Diffusion_Profile* Diffusion_Profile::create(const Asset_Import_Config* ass_imp_config)
	{
        Diffusion_Profile_Handle hnd = resource_cache.alloc_diffusion_profile(ass_imp_config);
        
		return &resource_cache.m_diffusion_profiles.at(hnd.index);
	}
	
	void Diffusion_Profile::load_asset(const Asset_Import_Config* asset_imp_conf)
	{
		if(!m_is_loaded)
		{
			deserialize();
			m_is_loaded = true;
		}
	}

    static int yaml_scalar_float_vec_event_initialize_and_emit(yaml_emitter_t* emitter, yaml_event_t* event, float* data, int num_components)
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

	int Diffusion_Profile::serialize()
	{
        yaml_emitter_t emitter;
        yaml_event_t event;

        yaml_emitter_initialize(&emitter);

        FILE* file = fopen(m_file_path.c_str(), "wb");

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

                yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"Textures", strnlen_s("Textures", MAX_PATH), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                if (!yaml_emitter_emit(&emitter, &event)) goto error;

                {
                    yaml_mapping_start_event_initialize(&event, NULL, (yaml_char_t*)YAML_MAP_TAG, 1, YAML_ANY_MAPPING_STYLE);
                    if (!yaml_emitter_emit(&emitter, &event)) goto error;

                    {
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"Subsurface Albedo", strlen("Subsurface Albedo"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                        int err = yaml_scalar_float_vec_event_initialize_and_emit(&emitter, &event, &sss_params.subsurface_albedo.x, sizeof(sss_params.subsurface_albedo)/sizeof(sss_params.subsurface_albedo.x));
                        if (!err) goto error;
                    }
                    {
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"Mean Free Path Color", strlen("Mean Free Path Color"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                        int err = yaml_scalar_float_vec_event_initialize_and_emit(&emitter, &event, &sss_params.mean_free_path_color.x, sizeof(sss_params.mean_free_path_color)/sizeof(sss_params.mean_free_path_color.x));
                        if (!err) goto error;
                    }
                    {
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"Mean Free Path Distance", strlen("Mean Free Path Distance"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                        int err = yaml_scalar_float_vec_event_initialize_and_emit(&emitter, &event, &sss_params.mean_free_path_dist, 1);
                        if (!err) goto error;
                    }
                    {
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"Scale", strlen("Scale"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                        sprintf(tmp_string, "%f", sss_params.scale);
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_FLOAT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;
                    }
                    {
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_STR_TAG, (yaml_char_t*)"Number Samples", strlen("Number Samples"), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;

                        sprintf(tmp_string, "%i", sss_params.num_samples);
                        yaml_scalar_event_initialize(&event, NULL, (yaml_char_t*)YAML_INT_TAG, (yaml_char_t*)tmp_string, strlen(tmp_string), 1, 0, YAML_PLAIN_SCALAR_STYLE);
                        if (!yaml_emitter_emit(&emitter, &event)) goto error;
                    }

                    yaml_mapping_end_event_initialize(&event);
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

    int Diffusion_Profile::deserialize()
    {
        //yaml_parser_t parser;
        //yaml_event_t event;

        //yaml_parser_initialize(&parser);

        //FILE* file = fopen(m_file_path.c_str(), "rb");
        //yaml_parser_set_input_file(&parser, file);

        //bool is_stream_ended = false;

        //do
        //{
        //    int status = yaml_parser_parse(&parser, &event);
        //    if (status == 0)
        //    {
        //        fprintf(stderr, "yaml_parser_parse error\n");
        //        is_stream_ended = true;
        //    }

        //    switch (event.type)
        //    {
        //    case (YAML_STREAM_END_EVENT):
        //    {
        //        is_stream_ended = true;
        //        break;
        //    }
        //    case (YAML_MAPPING_START_EVENT):
        //    {
        //        status = yaml_parser_parse(&parser, &event);
        //        assert(status != 0);
        //        if (event.type == YAML_SCALAR_EVENT)
        //        {
        //            if (strncmp((char*)event.data.scalar.value, "Textures", strnlen("Textures", MAX_PATH)) == 0)
        //            {
        //                status = yaml_parser_parse(&parser, &event); // MAPPING_START

        //                status = yaml_parser_parse(&parser, &event); // FIRST SCALAR
        //                do
        //                {
        //                    char tex_name[MAX_PATH] = {};
        //                    sprintf(tex_name, "%s", (char*)event.data.scalar.value);

        //                    status = yaml_parser_parse(&parser, &event);
        //                    uint64_t guid = 0;
        //                    sscanf((char*)event.data.scalar.value, "%" PRIu64, &guid);
        //                    Asset_With_Config* asset_with_conf = asset_registry.get_asset_by_guid(guid);

        //                    if (asset_with_conf != nullptr)
        //                    {
        //                        set_shader_resource_asset(resources, tex_name,
        //                            static_cast<Texture2D*>(asset_with_conf->asset),
        //                            static_cast<Texture_Import_Config*>(asset_with_conf->import_config)
        //                        );
        //                    } else
        //                    {

        //                        set_shader_resource_asset(resources, tex_name, nullptr, nullptr);
        //                    }

        //                    status = yaml_parser_parse(&parser, &event); // EITHER KEY SCALAR OR MAPPING END

        //                } while (event.type != YAML_MAPPING_END_EVENT);
        //            }

        //            status = yaml_parser_parse(&parser, &event);

        //            if (strncmp((char*)event.data.scalar.value, "_matPrms", strnlen("_matPrms", MAX_PATH)) == 0)
        //            {
        //                Constant_Buffer_Data* cb_data = nullptr;
        //                for (auto& resource : resources)
        //                {
        //                    if (resource.m_type == Resource_Type::ZRY_CBUFF) cb_data = resource.m_parameters;
        //                }

        //                status = yaml_parser_parse(&parser, &event); // MAPPING_START

        //                status = yaml_parser_parse(&parser, &event); // FIRST SCALAR

        //                int num_components = 0;
        //                float param_data[4];
        //                do
        //                {
        //                    char param_name[MAX_PATH] = {};
        //                    sprintf(param_name, "%s", (char*)event.data.scalar.value);

        //                    status = yaml_parser_parse(&parser, &event);
        //                    if (!status) return status;

        //                    status = yaml_parser_parse(&parser, &event);
        //                    if (!status) return status;

        //                    do
        //                    {
        //                        status = yaml_parser_parse(&parser, &event);
        //                        if (!status) return status;

        //                        //TODO: here assumes all parameters are float vectors
        //                        sscanf((char*)event.data.scalar.value, "%f", &param_data[num_components]);

        //                        num_components += 1;

        //                        status = yaml_parser_parse(&parser, &event);
        //                        if (!status) return status;
        //                    } while (event.type != YAML_MAPPING_END_EVENT);

        //                    bool is_var_set = set_constant_buff_var(cb_data, param_name, param_data, num_components * sizeof(float));
        //                    assert(is_var_set == true);

        //                    status = yaml_parser_parse(&parser, &event); // EITHER KEY SCALAR OR MAPPING END
        //                    num_components = 0;

        //                } while (event.type != YAML_MAPPING_END_EVENT);
        //            }
        //        }

        //        break;
        //    }
        //    case (YAML_SCALAR_EVENT):
        //    default:
        //    {
        //        break;
        //    }

        //    }

        //} while (!is_stream_ended);

        //m_is_loaded = true;
        //update_standard_internals(*this);

        //fclose(file);

        //yaml_event_delete(&event);
        //yaml_parser_delete(&parser);

        //TODO: return significant value
        return 0;
    }
}