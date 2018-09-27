# lasVR
Visualising point clouds in VR

lasVR is a collaboration between (NCI)[https://nci.org.au/] and the [TraitCapture](traitcapture.org) project, a joint project between from ANU node of the Australian Plant Phenomics Facility [(APPF)](http://www.plantphenomics.org.au) and the [Borevitz Lab](http://borevitzlab.anu.edu.au) at ANU.

LasVR provides an easy to use, drag and drop interface for viewing and interacting with singe and time-series pointclouds of unlimited size on desktop and with the Vive or Oculus. 

Basic annotations are supported in the current Vive version. 
(Any georeferenced numeric data or icons can be added manually by creating an "annotation.json" file matching the format used for annotations added in-program.).

You can also very easily generate fly-through movies of the point clouds. 
Example fly-through movie: https://youtu.be/OXQyATxcxTM

LasVR works with both Vive and Oculus, but performance is better with a Vive and annotations are not fully supported in the Oculus.

Point cloud of the National Arboretum in Canberra, ACT, Australia:
![Sample screen capture in desktop mode](/media/Shristi-screen-cap-1.jpg)

Point cloud of a sorghum crop LiDAR scan from the TERRA-REF site in Maricopa, AZ, USA:
![Sample screen capture in desktop mode](/media/3D-point-cloud-of-maricopa-cropped.jpg)

*Data Format*
* Point clouds must be processed into the [potree format](https://github.com/potree/PotreeConverter/releases)
* The Potree converter can read LAZ,LAS and PLY files (other formats can usually be converted using [CloudCompare](http://www.cloudcompare.org).

*Instructions*
* Unzip with 7-zip
* Navigate to the _/bin/_ folder in the Shirshti folder and open _shrishti.exe_
* Drag and drop a potree formatted point cloud dataset _UQ-Time-Series_ folder onto Shrishti window
   * Wait a bit for it to load. It may appear frozen for a few seconds. A dialog box pops up that you have to click ok on; sometimes it get blocked by another window so after you've waited about 15-sec, click on Shristi again or alt-tab away from it and then back again, and it should pop up.

*TimeSeries data*
* LasVR can read point clouds of unlimited size. For extremely large point clouds, the first time you load there will be a few minutes delay while the dataset is ingested.
* For time-series data, put all folders into a parent folder and drag and drop the parent folder onto LasVR

*Navigation (desktop)*
* Use L/R mouse buttons for navigation
* Mouse wheel to zoom in/out
* L/R Arrow keys to navigate through time
* Up/Down Arrow keys to change point size
* Menu->Options->FlyMode to use the mouse to fly around

*Point Budget*
Use: Menu->Options->PointBudget to reduce point budget depending on your graphics card. 
* If you don't have a good graphics card, you can reduce that to 1 million but you will still need a pretty good GPU. If you have a new 1080 or higher, you can push it up to 10-million or more

*VR*
* If you have VR, use Menu->Options->VRMode to enable VR mode.
* Before running VR, make sure SteamVR is launched and all components (controller, trackers, hand controller) are functional and not grayed out.

*Navigation (VR)*
* Vive: use the right touchpad to fly 
* Oculus: Use the right thumbstick to fly (you don't need to push it hard, just subtle movements)
* Move your right hand by bending your wrist to navigate as you fly around. Sometimes it is easier to point your hand down or up and fly backwards away to get to where you are going
* Left hand: 
   - If you can't see anything or things look weird, try clicking on the RESET button -> Use your right hand to point at the left menu item and click with your left trigger finger to select
   - Left/Right on the left thumbstick should let you toggle between the menu and the map. In map mode, if you point at a spot with your right hand and click the right trigger, you will jump to that spot
   - Sometimes the left thumbstick doesn't shift to the map, try clicking the "map" icon to the left of the RESET button
   - Change "point size" to make the points bigger or smaller
   - If you don't have a great GPU, deselect "soft shadow" and "edges"
   - Pushing the left thumbstick left/right toggles between the map, the icons for annotation and the menu

----------------------------------------------------------------
Notes on top.json settings:
* gravity : true = Ground following (tries to prevent you from going through the ground)
* ground_height : 1.8  = Height of user when at ground level
* teleport_scale : 1.0  = Affects travel speed. Keep to around 1
