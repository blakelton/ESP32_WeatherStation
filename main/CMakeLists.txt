idf_component_register(SRCS "dht22.c" "main.c" "rgb_led.c" "wifi_app.c" "http_server.c" "app_nvs.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES webpage/app.css webpage/app.js webpage/index.html webpage/jquery-3.3.1.min.js webpage/favicon.ico)
