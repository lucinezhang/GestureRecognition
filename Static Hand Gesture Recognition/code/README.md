gesturejson.txt -- Output each type's probability to this file, read data from this file to create the histogram


A.PNG,C.PNG,D.PNG,U.PNG,V.PNG -- Images of five hand gestures

Nothing.png -- Image shown when it's none of the five types


static_gesture_5_acc.jpg -- Training accuracy of CNN

static_gesture_5_loss.jpg -- Training loss of CNN


get_data.py -- Get training data

train_static_gesture_5.py -- Main code of CNN

predict_gesture.py & gestureCNN.py -- Get hand gesture from the front camera of laptop and predict it

liveplot.py -- Plot the histogram according to the probabilities of five gestures


static_gesture_5.h5 -- Model's weights of a trained CNN