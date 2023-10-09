set( srcs
      source/ota_ws_update_http.c 
)
set( includedir
      include
)
set( priv_includedir 
      private_include
    )
set(require 
 )
set( priv_require
      app_update
      esp_http_server
      mbedtls
)
set( embed_file 
      source/ota_ws_update.html
)
set( embed_txt_file 
)

if(CONFIG_OTA_PRE_ENCRYPTED_MODE)
  list(APPEND srcs 
      source/ota_ws_update_esp_preencrypted.c
      esp_encrypted_img/src/esp_encrypted_img.c
  )
  list(APPEND priv_includedir
      esp_encrypted_img/include
  )
  if(CONFIG_OTA_PRE_ENCRYPTED_RSA_KEY_ON_COMPONENT_LOCATION)
    list(APPEND embed_txt_file 
        ${COMPONENT_DIR}/${CONFIG_OTA_PRE_ENCRYPTED_RSA_KEY_DIRECTORY}/private.pem
    )
   else()
    list(APPEND embed_txt_file 
        ${project_dir}/${CONFIG_OTA_PRE_ENCRYPTED_RSA_KEY_DIRECTORY}/private.pem
    )
   endif() 
else()
  list(APPEND srcs 
      source/ota_ws_update_esp.c
  )
endif()

idf_component_register(
                    SRCS ${srcs}
                    INCLUDE_DIRS ${includedir}
                    PRIV_INCLUDE_DIRS ${priv_includedir}
                    REQUIRES ${require}
                    PRIV_REQUIRES ${priv_require}
                    EMBED_FILES ${embed_file}
                    EMBED_TXTFILES ${embed_txt_file} 
)

if(CONFIG_OTA_PRE_ENCRYPTED_MODE)

    include(esp_encrypted_img/project_include.cmake)
    #redefine ESP_IMG_GEN_TOOL_PATH
    set(ESP_IMG_GEN_TOOL_PATH ${CMAKE_CURRENT_LIST_DIR}/esp_encrypted_img/tools/esp_enc_img_gen.py)

    #openssl genrsa -out rsa_key/private.pem 3072
   
    if(CONFIG_OTA_PRE_ENCRYPTED_RSA_KEY_ON_COMPONENT_LOCATION)
    # for rsa_key on components project_dir
    create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
      ${COMPONENT_DIR}/${CONFIG_OTA_PRE_ENCRYPTED_RSA_KEY_DIRECTORY}/private.pem ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)
    else()  
    # for rsa_key on project dir  
    create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
      ${project_dir}/${CONFIG_OTA_PRE_ENCRYPTED_RSA_KEY_DIRECTORY}/private.pem ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)
    endif()  
endif()
