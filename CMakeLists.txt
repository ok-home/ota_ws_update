if(CONFIG_OTA_PRE_ENCRYPTED_MODE)
  idf_component_register(
                    SRCS 
                      source/ota_ws_update_esp_preencrypted.c
                      source/ota_ws_update_http.c 
                    INCLUDE_DIRS 
                      include
                    PRIV_INCLUDE_DIRS
                      private_include
                    REQUIRES
                      esp_http_server
                      app_update
                      esp_encrypted_img
                      #esp_wifi
                    EMBED_FILES 
                      source/ota_ws_update.html
                    EMBED_TXTFILES  
                      rsa_key/private.pem
  )
    #openssl genrsa -out rsa_key/private.pem 3072
    #create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    #  ${project_dir}/../rsa_key/private.pem ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)
    # for rsa_key on components dir  ?
    create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
      ${COMPONENT_DIR}/rsa_key/private.pem ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)
    # for rsa_key on project dir  ?
    #create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    #  ${project_dir}/rsa_key/private.pem ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)

else()
  idf_component_register(
                    SRCS 
                      source/ota_ws_update_esp.c
                      source/ota_ws_update_http.c 
                    INCLUDE_DIRS 
                      include
                    PRIV_INCLUDE_DIRS
                      private_include
                    REQUIRES
                      esp_http_server
                      app_update
                      #esp_wifi
                    EMBED_FILES 
                      source/ota_ws_update.html
  )
endif()