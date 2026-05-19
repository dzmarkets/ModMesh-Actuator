idf_component_register(SRCS "actuators.c"
                    INCLUDE_DIRS "include"
                    REQUIRES shared_config driver mesh_manager status_indicator esp_system)
