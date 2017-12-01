# lasVR
Visualising point clouds in VR

lasVR is a collaboration between NCI and the [TraitCapture](traitcapture.org) project from ANU node of the Australian Plant Phenomics Facility [(APPF)](http://www.plantphenomics.org.au) and the [Borevitz Lab](http://borevitzlab.anu.edu.au).

LasVR provides an easy to use, drag and drop interface for viewing and interacting with singe and time-series pointclouds of unlimited size on desktop and with the Vive or Oculus. 

We will also be adding tha ability to co-visualize any associated numeric data with the point clouds (i.e. sensor data, object labels, etc.).

You can also very easily generate fly-through movies of the point clouds. 
Example fly-through movie: https://youtu.be/OXQyATxcxTM



![Sample screen capture in desktop mode](/media/Shristi-screen-cap-1.jpg)


Instructions
* Unzip with 7-zip
* Run /Srishti_v1.04/bin/shrishti.exe 
* Drag and drop the UQ-Time-Series folder onto Shrishti window
   * Wait a bit for it to load. It may appear frozen for a few seconds. A dialog box pops up that you have to click ok on; sometimes it get blocked by another window so after you've waited about 15-sec, click on Shristi again or alt-tab away from it and then back again, and it should pop up.

Navigation
* Use L/R mouse buttons for navigation
* Mouse wheel to zoom in/out
* L/R Arrow keys to navigate through time
* Up/Down Arrow keys to change point size
* Menu->Options->FlyMode to use the mouse to fly around

Point Budget:
Use: Menu->Options->PointBudget to reduce point budget depending on your graphics card. 
* If you don't have a good graphics card, you can reduce that to 1 million but you will still need a pretty good GPU. If you have a new 1080 or higher, you can push it up to 10-million or more

VR:
* If you have VR, use Menu->Options->VRMode to enable VR mode.
* Before running VR, make sure SteamVR is launched and all components (controller, trackers, hand controller) are functional and not grayed out
* Be patient with it, it takes some time to load and may crash.
* If you get an VR_not found or similar error after a crash, you may need a restart before it will work agian.

In VR:
* Use the right thumbstick to fly (you don't need to push it hard, just subtle movements)
* Move your right hand by bending your wrist to navigate as you fly around. Sometimes it is easier to point your hand down or up and fly backwards away to get to where you are going
* Left hand: 
   - If you can't see anything or things look weird, try clicking on the RESET button -> Use your right hand to point at the left menu item and click with your left trigger finger to select
   - Left/Right on the left thumbstick should let you toggle between the menu and the map. In map mode, if you point at a spot with your right hand and click the right trigger, you will jump to that spot
   - Sometimes the left thumbstick doesn't shift to the map, try clicking the "map" icon to the left of the RESET button
   - Change "point size" to make the points bigger or smaller
   - If you don't have a great GPU, deselect "soft shadow" and "edges"

----------------------------------------------------------------
Notes on top.json settings:
* gravity : true = Ground following (tries to prevent you from going through the ground)
* ground_height : 1.8  = Height of user when at ground level
* teleport_scale : 1.0  = Affects travel speed. Keep to around 1
