# Embed the server root certificate into the final binary

idf_component_register(
                    SRCS 
                        "simple_ota_example.c"
                    INCLUDE_DIRS 
                        "."
                    EMBED_FILES 
                        
)