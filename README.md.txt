This project was for ME 106: Intro to Mechatronics, at San Jose State University, CA.

Our initial idea was to use accelerometers to track velocity and ultimately position.
We soon realized that we could not obtain acceleration data for such small movements
from the Wii Remote (At least not without the Wii motion plus with a gyroscope). We 
then decided to use the camera on the Wii remote to track two IR LED's on the front
of the cart. This worked pretty well. Ultimately I would like to combine this project
with acceleration data.

We used the Wii library written for the USB shield made by circuits@home.com. We wrote code
to access the camera.

Another way I would like to explore is using the Xbox connect motion tracking capabilities
instead of the wii

Check out our project report to see the circuit diagram.

In order to run the code, you will need to put this entire library in your Arduino library folder,
and rename without the period for it to work. https://github.com/felis/USB_Host_Shield_2.0

Email if interested!
Allan Glover
adglover9.81@gmail.com


Big thanks to Kristian Lauszus. He was responsible for writing much of the Wii library, 
and helped a ton with writing the code to access the Wii IR camera