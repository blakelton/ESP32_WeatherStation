idf_component_register(SRCS "main.c" "rgb_led.c" "wifi_app.c" "http_server.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES webpage/app.css webpage/app.js webpage/index.html webpage/jquery-3.3.1.min.js webpage/favicon.ico)
