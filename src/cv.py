try: 
    import cv2
    import numpy as np
    _CV_AVAILABLE = True
except ImportError:
    _CV_AVAILABLE = False

def classify_object(image, weight):
    if not _CV_AVAILABLE:
        if (weight >= 2):
            return True
        else:
            return False
    else:
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blurred = cv2.medianBlur(gray, 5)
    circles = cv2.HoughCircles(
        blurred, cv2.HOUGH_GRADIENT, dp=1, minDist=50,
        param1=100, param2=30, minRadius=10, maxRadius=40
    )
    return circles is not None
