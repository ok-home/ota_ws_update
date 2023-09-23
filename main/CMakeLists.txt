# Embed the server root certificate into the final binary

idf_component_register(
                    SRCS 
                      example_ota_ws.c 
                    INCLUDE_DIRS 
                        "."
                    #EMBED_FILES 
                        ota_ws.html
)