idf_component_register(SRCS "main.c"
                            "link.c"
                            "camera.c"
                            "storage.c"
                            "preprocess.c"
                            "classify_blob.c"
                            "classify_model.cpp"
                       INCLUDE_DIRS "." "../../shared")
