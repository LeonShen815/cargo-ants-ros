Header header            # timestamp in the header is the acquisition time of
                         # the first ray in the scan.
                         #
                         # in frame frame_id, angles are measured around
                         # the positive Z axis (counterclockwise, if Z is up)
                         # with zero angle being forward along the x axis

uint8 sensorId    # id of sensor

float32 vnorth
float32 veast
float32 vdown
float32 speed2d
