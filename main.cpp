#include <iostream>
#include <fstream>

#include "vis.h"
#include "world.h"

#include <SDL.h>
#include <SDL_opengl.h>

#ifdef __APPLE__
//why, apple?   why????
#include <OpenGL/glu.h>
#else
#include <gl/glu.h>
#endif

void loop(SDL_Window* window,
		  World& world);

bool dumpFrames;
bool dumpColors;
int main(int argc, char** argv){
  
  if(argc < 2){
	std::cout << "usage: ./runSimulator <inputfile.json> [writeSomethinHereIfYouWantToDumpFrames, 'color' will dump colors, anything else won't]" << std::endl;
	exit(1);
  }
  
  dumpFrames = (argc == 3);
  dumpColors = dumpFrames && (std::string("color") == argv[2]);

  World world;
  world.loadFromJson(argv[1]);
  world.initializeNeighbors();

  //exit(0);

  if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
	std::cout << "couldn't init SDL" << std::endl;
	exit(1);
  }
  
  std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> 
	window{SDL_CreateWindow("3.2", SDL_WINDOWPOS_CENTERED,
							SDL_WINDOWPOS_CENTERED,
							800,800,
							SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN),
	  &SDL_DestroyWindow};
  //automatically destory the window when we're done

  SDL_SetWindowSize(window.get(), 800, 800);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  
  loop(window.get(), world);
  return 0;
}


void loop(SDL_Window* window, World& world){

  
  
  auto context = SDL_GL_CreateContext(window);  
  int frame = 0;

  Camera camera;
  VisSettings visSettings;
  visSettings.drawClusters = false;
  visSettings.drawFracturePlanes = false;
  std::ifstream eyeIn(".eye.txt");
  if(eyeIn){
	Eigen::Vector3d eye;
	eyeIn >> eye.x() >> eye.y() >> eye.z();
	camera.position = eye;
  } 



  benlib::Profiler totalProf;
  {
	auto totalTime  = totalProf.timeName("everything");
  //loop
  bool readyToExit = false;

  bool mouseDown = false;
  Eigen::Vector2i mousePosition;

  bool paused = false;
  while(!readyToExit){

	SDL_Event event;
	while(SDL_PollEvent(&event)){
	  switch(event.type){
	  case SDL_KEYDOWN:
		if(event.key.keysym.sym == SDLK_ESCAPE){
		  readyToExit = true;
		} else if(event.key.keysym.sym == SDLK_r){
		  world.restart();
		} else if(event.key.keysym.sym == SDLK_UP){
		  camera.move(true);
		} else if(event.key.keysym.sym == SDLK_DOWN){
		  camera.move(false);
      } else if(event.key.keysym.sym == SDLK_RIGHT){
		  visSettings.which_cluster++;
        if (visSettings.which_cluster >= world.clusters.size()) {
           visSettings.which_cluster = world.clusters.size()-1;
        }
        std::cout << "Displaying cluster: " << visSettings.which_cluster << std::endl;
		} else if(event.key.keysym.sym == SDLK_LEFT){
		  visSettings.which_cluster--;
        if (visSettings.which_cluster < 0) {
           visSettings.which_cluster = -1;
        }
        std::cout << "Displaying cluster: " << visSettings.which_cluster << std::endl;
		} else if(event.key.keysym.sym == SDLK_c){
		  visSettings.drawClusters = !visSettings.drawClusters;
      } else if(event.key.keysym.sym == SDLK_f){
		  visSettings.drawFracturePlanes = !visSettings.drawFracturePlanes;
      } else if(event.key.keysym.sym == SDLK_j){
		  visSettings.joshDebugFlag = !visSettings.joshDebugFlag;
      } else if(event.key.keysym.sym == SDLK_v){
		  visSettings.drawColoredParticles = !visSettings.drawColoredParticles;
      } else if(event.key.keysym.sym == SDLK_t){
		  visSettings.colorByToughness = !visSettings.colorByToughness;
      } else if(event.key.keysym.sym == SDLK_d){
		  world.dragWithPlanes = !world.dragWithPlanes;  
		} else if(event.key.keysym.sym == SDLK_p){
		  paused = !paused;
      }
		break;
	  case SDL_QUIT:
		readyToExit = true;
		break;

	  case SDL_MOUSEBUTTONDOWN:
		mouseDown = true;
		mousePosition = Eigen::Vector2i{event.button.x, event.button.y};
		break;
	  case SDL_MOUSEBUTTONUP:
		mouseDown = false;
		mousePosition = Eigen::Vector2i{event.button.x, event.button.y};
		break;
		
	  case SDL_MOUSEWHEEL:
		camera.zoom(event.wheel.y);
		break;
	  case SDL_MOUSEMOTION:
		if(mouseDown){
		  Eigen::Vector2i newPosition{event.motion.x, event.motion.y};
		  camera.pan(mousePosition, newPosition);
		  mousePosition = newPosition;
		}
		break;
	  default:
		; // do nothing
	  }
	  std::ofstream eyeOuts(".eye.txt");
	  eyeOuts << camera.position.x() << ' ' 
			  << camera.position.y() << ' ' 
			  << camera.position.z() << std::endl;

	  if(readyToExit){break;}
	}

	//uncomment to save output files
	/*
	char fname[80];
	sprintf (fname, "particles.%05d.txt", 10000+(frame++));
	world.saveParticleFile(std::string(fname));
	if(frame > 600){break;}
	*/
	
   if(paused){
	  //world.drawSingleCluster(window, frame);
	  //SDL_Delay(300);
	 drawWorldPretty(world, camera, visSettings, window);
	} else {
	  world.timestep();
	  auto renderTime = totalProf.timeName("render");
	  drawWorldPretty(world, camera, visSettings, window);

	  if(dumpFrames){
		world.dumpParticlePositions(std::string("frames/particles.") + std::to_string(frame) + ".txt");
		world.dumpClippedSpheres(std::string("frames/particles.") + std::to_string(frame) + ".txt.spheres");
		if(dumpColors){
		  world.dumpColors(std::string("frames/particles.") + std::to_string(frame) + ".txt.colors");
		}
	  }
	  ++frame;
	  if(frame % 60 == 0){std::cout << frame << std::endl;}
	
	// Uncomment the following for uniform-length clips
/*
	  if (frame == 150)
	{
		readyToExit = true;
		break;
	}
*/	  
   }
   
   
  }
  }
  world. prof.dump<std::chrono::duration<double>>(std::cout);
  
  totalProf.dump<std::chrono::duration<double>>(std::cout);

  std::cout << "total frames: " << frame << std::endl;
  SDL_GL_DeleteContext(context);
}
