idf_component_register(SRCS "vision.c" "power.c" "main.c"
                            "link.c"
                            "camera.c"
                            "storage.c"
                            "preprocess.c"
                            "classify_blob.c"
                            "classify_model.cpp"
                            #"capture_test.c"   # bench capture test: uncomment with its two main.c lines
                            "../../shared/vision_protocol.c"
                       INCLUDE_DIRS "." "../../shared")
