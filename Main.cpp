//===========================================================================
/*
    CS277 - Experimental Haptics
    Winter 2010, Stanford University

    You may use this program as a boilerplate for starting your homework
    assignments.  Use CMake (www.cmake.org) on the CMakeLists.txt file to
    generate project files for the development tool of your choice.  The
    CHAI3D library directory (chai3d-2.1.0) should be installed as a sibling
    directory to the one containing this project.

    These files are meant to be helpful should you encounter difficulties
    setting up a working CHAI3D project.  However, you are not required to
    use them for your homework -- you may start from anywhere you'd like.

    \author    Francois Conti & Sonny Chan
    \date      January 2010
*/
//===========================================================================

//---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//---------------------------------------------------------------------------
#include "chai3d.h"

#include "Assignment.h"

#include "1_HelloWorld.h"
#include "2_ReadDevicePosition.h"
#include "3_BasicForceEffects.h"
#include "4_HapticWall.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W         = 600;
const int WINDOW_SIZE_H         = 600;

// mouse menu options (right button)
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;

// maximum number of haptic devices supported in this demo
const int MAX_DEVICES           = 8;


//---------------------------------------------------------------------------
// DECLARED VARIABLES
//---------------------------------------------------------------------------

std::vector<Assignment*> assignments;
volatile size_t currentAssignment = 0;

// a world that contains all objects of the virtual environment
cWorld* world = 0;

// a camera that renders the world in a window display
cCamera* camera;

// a light source to illuminate the objects in the virtual scene
cLight *light;

// width and height of the current window display
int displayW  = 0;
int displayH  = 0;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the first haptic device detected on this computer
cGenericHapticDevice* hapticDevice = 0;

// labels to show haptic device position and update rate
cLabel* positionLabel = 0;
cLabel* rateLabel = 0;
cLabel* assignmentLabel = 0;
double assignmentLabelWidth;
double rateEstimate = 0;


// status of the main simulation haptics loop
bool simulationRunning = false;

// has exited haptics simulation thread
bool simulationFinished = false;

//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// callback when the window display is resized
void resizeWindow(int w, int h);

// callback when a keyboard key is pressed
void keySelect(unsigned char key, int x, int y);

// callback when the right mouse button is pressed to select a menu item
void menuSelect(int value);

// function called before exiting the application
void close(void);

// main graphics callback
void updateGraphics(void);

// main haptics loop
void updateHaptics(void);

void reset(size_t assignmentId);


//===========================================================================
/*
    This application illustrates the use of the haptic device handler
    "cHapticDevicehandler" to access haptic devices connected to the computer.

    In this example the application opens an OpenGL window and displays a
    3D cursor for the first device found. If the operator presses the device
    user button, the color of the cursor changes accordingly.

    In the main haptics loop function  "updateHaptics()" , the position and 
    user switch status of the device are retrieved at each simulation iteration.
    This information is then used to update the position and color of the
    cursor. A force is then commanded to the haptic device to attract the 
    end-effector towards the device origin.
*/
//===========================================================================

int main(int argc, char* argv[])
{
    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------

    printf ("\n");
    printf ("Based on:\n");
    printf ("-----------------------------------\n");
    printf ("CS277 - Experimental Haptics\n");
    printf ("Homework Boilerplate Application\n");
    printf ("January 2010, Stanford University\n");
    printf ("-----------------------------------\n");
    printf ("\n\n");

    //-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------

    // create a new world.

    // set the background color of the environment
    // the color is defined by its (R,G,B) components.

    assignments.push_back(new HelloWorld());
    assignments.push_back(new ReadDevicePosition());
    assignments.push_back(new BasicForceEffects());
    assignments.push_back(new HapticWall());


    //cShapeLine* myLine = new cShapeLine(cVector3d(0, 0.02, 1), cVector3d(0,0.02,-1));
    //world->addChild(myLine);
    //-----------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //-----------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // read the number of haptic devices currently connected to the computer
    int numHapticDevices = handler->getNumDevices();

    // if there is at least one haptic device detected...
    if (numHapticDevices)
    {
        // get a handle to the first haptic device
        handler->getDevice(hapticDevice);

        // open connection to haptic device
        hapticDevice->open();

		// initialize haptic device
		hapticDevice->initialize();

        // retrieve information about the current haptic device
        cHapticDeviceInfo info = hapticDevice->getSpecifications();

    }



    //-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------

    // initialize GLUT
    glutInit(&argc, argv);

    // retrieve the resolution of the computer display and estimate the position
    // of the GLUT window so that it is located at the center of the screen
    int screenW = glutGet(GLUT_SCREEN_WIDTH);
    int screenH = glutGet(GLUT_SCREEN_HEIGHT);
    int windowPosX = (screenW - WINDOW_SIZE_W) / 2;
    int windowPosY = (screenH - WINDOW_SIZE_H) / 2;

    //Initialize world
    reset(0);

    // initialize the OpenGL GLUT window
    glutInitWindowPosition(windowPosX, windowPosY);
    glutInitWindowSize(WINDOW_SIZE_W, WINDOW_SIZE_H);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(updateGraphics);
    glutKeyboardFunc(keySelect);
    glutReshapeFunc(resizeWindow);
    glutSetWindowTitle("DH2660 Haptics Lab 1, KTH");

    // create a mouse menu (right button)
    glutCreateMenu(menuSelect);
    glutAddMenuEntry("full screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("window display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    //-----------------------------------------------------------------------
    // START SIMULATION
    //-----------------------------------------------------------------------

    // simulation in now running
    simulationRunning = true;

    // create a thread which starts the main haptics rendering loop
    cThread* hapticsThread = new cThread();
    hapticsThread->set(updateHaptics, CHAI_THREAD_PRIORITY_HAPTICS);

    // start the main graphics rendering loop
    glutMainLoop();

    // close everything
    close();

    // exit
    return (0);
}

//---------------------------------------------------------------------------

void reset(size_t assignmentId)
{
    assignments[currentAssignment]->setInitialized(false);
    currentAssignment = assignmentId;

    delete world;
    world = new cWorld();
    world->setBackgroundColor(0.0, 0.0, 0.0);

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and oriente the camera
    camera->set( cVector3d (0.2, 0.0, 0.0),    // camera position (eye)
                 cVector3d (0.0, 0.0, 0.0),    // lookat position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the "up" vector

    // set the near and far clipping planes of the camera
    // anything in front/behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);

    // create a light source and attach it to the camera
    light = new cLight(world);
    camera->addChild(light);                   // attach light to camera
    light->setEnabled(true);                   // enable light source
    light->setPos(cVector3d( 2.0, 0.5, 1.0));  // position the light source
    light->setDir(cVector3d(-2.0, 0.5, 1.0));  // define the direction of the light beam

    // create a label that shows the haptic loop update rate
    rateLabel = new cLabel();
    camera->m_front_2Dscene.addChild(rateLabel);

    positionLabel = new cLabel();
    positionLabel->setPos(8, 8, 0);
    camera->m_front_2Dscene.addChild(positionLabel);

    assignmentLabel = new cLabel();
    camera->m_front_2Dscene.addChild(assignmentLabel);

    assignments[currentAssignment]->initialize(world, camera);
    assignments[currentAssignment]->setInitialized(true);

    assignmentLabel->m_string = assignments[currentAssignment]->getName();
    assignmentLabel->m_font->setPointSize(19.0f);

    assignmentLabelWidth = 0.0;
    for(size_t i = 0; i < assignmentLabel->m_string.size(); ++i)
        assignmentLabelWidth += assignmentLabel->m_font->getCharacterWidth(assignmentLabel->m_string[i]);
}

//---------------------------------------------------------------------------

void loadAssignment(size_t assignmentIndex)
{
    if(assignmentIndex >= assignments.size())
        return;

    currentAssignment = assignmentIndex;
}

//---------------------------------------------------------------------------

void resizeWindow(int w, int h)
{
    // update the size of the viewport
    displayW = w;
    displayH = h;
    glViewport(0, 0, displayW, displayH);
}

//---------------------------------------------------------------------------

void keySelect(unsigned char key, int x, int y)
{
    // escape key
    if ((key == 27) || (key == 'x'))
    {
        // close everything
        close();

        // exit application
        exit(0);
    }

    // Key 1 - 9 corresponding to an existing assignment
    if(key >= '1' && key < '1' + assignments.size())
    {
        reset(key - '1');
    }
}

//---------------------------------------------------------------------------

void menuSelect(int value)
{
    switch (value)
    {
        // enable full screen display
        case OPTION_FULLSCREEN:
            glutFullScreen();
            break;

        // reshape window to original size
        case OPTION_WINDOWDISPLAY:
            glutReshapeWindow(WINDOW_SIZE_W, WINDOW_SIZE_H);
            break;
    }
}

//---------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close the haptic devices
    if (hapticDevice)
    {
        hapticDevice->close();
    }
}

//---------------------------------------------------------------------------

void updateGraphics(void)
{
    // Update the label showing the position of the haptic device
    if (hapticDevice)
    {
        cVector3d position;
        hapticDevice->getPosition(position);

        position = position * 1000.0; // Convert to mm
        char buffer[128];
        sprintf(buffer, "Device position: (%.2lf, %.2lf, %.2lf) mm", position.x, position.y, position.z);

        //Set the text to the label
        positionLabel->m_string = buffer;
    }

    if(assignments[currentAssignment]->isInitialized())
        assignments[currentAssignment]->updateGraphics();

    // Update the label with the haptic refresh rate
    char buffer[128];
    sprintf(buffer, "Haptic rate: %.0lf Hz", rateEstimate);
    rateLabel->m_string = buffer;
    rateLabel->setPos(displayW - 120, 8, 0);

    assignmentLabel->setPos(0.5 * (displayW - assignmentLabelWidth), displayH - 20, 0);

    // render world
    camera->renderView(displayW, displayH);

    // Swap buffers
    glutSwapBuffers();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    // inform the GLUT window to call updateGraphics again (next frame)
    if (simulationRunning)
    {
        glutPostRedisplay();
    }
}

//---------------------------------------------------------------------------

void updateHaptics(void)
{
    // a clock to estimate the haptic simulation loop update rate
    cPrecisionClock pclock;
    pclock.setTimeoutPeriodSeconds(1.0);
    pclock.start(true);

    cPrecisionClock clock;
    clock.start(true);
    int counter = 0;

    cPrecisionClock frameClock;
    frameClock.start(true);

    // main haptic simulation loop
    while(simulationRunning)
    {
        if (!hapticDevice)
            continue;

        double totalTime = clock.getCurrentTimeSeconds();

        double timeStep = frameClock.getCurrentTimeSeconds();
        frameClock.start(true);

        if(assignments[currentAssignment]->isInitialized())
            assignments[currentAssignment]->updateHaptics(hapticDevice, timeStep, totalTime);

        // Estimate the refresh rate
        ++counter;
        if (pclock.timeoutOccurred()) {
            pclock.stop();
            rateEstimate = counter;
            counter = 0;
            pclock.start(true);
        }
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//---------------------------------------------------------------------------
