
#include <memory>
#include <iostream>
#include <OgreRoot.h>
#include <OgreOctreePlugin.h>
#include <OgreGLPlugin.h>
#include <OgreWindowEventUtilities.h>
#include <OgreRenderWindow.h>
#include <OgreCamera.h>
#include <OgreViewport.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgreFreeimageCodec.h>

#include "world.h"
#include "color_spaces.h"
#include "particle.h"

int main(int argc, char** argv){
  if(argc < 2){
	std::cout << "usage: ./ogreApp <input.json>" << std::endl;
	return 1;
  }
  

  bool dumpFrames = argc > 2;
  

  auto octreePlugin = std::unique_ptr<Ogre::OctreePlugin>(new Ogre::OctreePlugin());
  auto glPlugin = std::unique_ptr<Ogre::GLPlugin>(new Ogre::GLPlugin());
  //the top two must outlive root
  auto ogreRoot = std::unique_ptr<Ogre::Root>(new Ogre::Root("","","ogreLog.log"));

  Ogre::FreeImageCodec::startup();

  
  ogreRoot->installPlugin(octreePlugin.get());
  octreePlugin->initialise();


  ogreRoot->installPlugin(glPlugin.get());
  glPlugin->initialise();
  
  std::cout << "installed plugins: " << std::endl;
  for(const auto& plugin : ogreRoot->getInstalledPlugins()){
	std::cout << '\t' << plugin->getName() << std::endl;
  }

  auto* sceneManager = ogreRoot->createSceneManager("OctreeSceneManager");

  auto renderers = ogreRoot->getAvailableRenderers();
  std::cout << "renderers: " << std::endl;
  for(auto & r : renderers){
	std::cout << '\t' << r->getName() << std::endl;
  }
  
  Ogre::RenderSystem* rs =
	ogreRoot->getRenderSystemByName("OpenGL Rendering Subsystem");

  assert(rs);
  if(!(rs->getName() == "OpenGL Rendering Subsystem")){
	throw std::runtime_error("couldn't set up render sys");
  }

  rs->setConfigOption("Full Screen", "No");
  rs->setConfigOption("VSync", "No");
  rs->setConfigOption("Video Mode", "960 x 540 @ 32-bit");

  ogreRoot->setRenderSystem(rs);

  auto* window = ogreRoot->initialise(true, "Ductile Fracture for Shape Matching");

  sceneManager->setAmbientLight(Ogre::ColourValue(0.1, 0.1, 0.1));

  auto camera = sceneManager->createCamera("theCamera");
  //todo use stuff from runSimulator
  camera->setPosition(Ogre::Vector3(0,0,3));
  camera->lookAt(Ogre::Vector3(0,0,0));
  camera->setNearClipDistance(0.5);

  auto* viewport = window->addViewport(camera);
  viewport->setBackgroundColour(Ogre::ColourValue(0,0,0));

  camera->setAspectRatio(
	  Ogre::Real(viewport->getActualWidth()) /
	  Ogre::Real(viewport->getActualHeight()));

  //load the meshes/textures/etc
  Ogre::ResourceGroupManager::getSingleton().
	addResourceLocation("/Users/ben/libs/ogre/Samples/Media/models", "FileSystem", "General");
  Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();


  auto* keyLight = sceneManager->createLight("key");
  keyLight->setPosition(20, 15, 20);


  //load stuff
  World world;
  world.loadFromJson(argv[1]);
  world.initializeNeighbors();

  const double sphereSize = 0.05;
  const double scaleFactor = sphereSize/200.0;

  //SM should outlive particles...
  auto cleanupParticle =
	[&sceneManager](Particle& p){
	/*if(p.sceneNode != nullptr){
	  p.sceneNode->setVisible(false);
	  }*/
	
	if(p.entity == nullptr){return;}
	p.sceneNode->detachObject(p.entity);
	sceneManager->destroySceneNode(p.sceneNode);
	sceneManager->destroyEntity(p.entity);
	//std::cout << "cleaning up a particle " << std::endl;
	
	
  };
  
  for(auto& p : world.particles){
	p.cleanup = cleanupParticle;
	p.sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
	p.sceneNode->setScale(scaleFactor, scaleFactor, scaleFactor);
	p.sceneNode->setPosition(p.position.x(), p.position.y(), p.position.z());
	p.entity = sceneManager->createEntity("sphere.mesh");
	p.sceneNode->attachObject(p.entity);
	auto material = Ogre::MaterialManager::getSingleton().create(
		"aMat",
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	//find closest color:
	/*	auto closestCluster = std::min_element(
		p.clusters.begin(), p.clusters.end(),
		[&p,&world](int a, int b){
		  return (p.position - world.clusters[a].worldCom).squaredNorm() <
		  (p.position - world.clusters[b].worldCom).squaredNorm();
		});
	
		RGBColor rgb = HSLColor(2.0*acos(-1)*(*closestCluster%12)/12.0, 0.7, 0.7).to_rgb();*/
	material->getTechnique(0)->setDiffuse(p.color.r, p.color.g, p.color.b, 1);
	p.entity->setMaterial(material);
  }
  
  bool readyToExit = false;

  std::string fileBase = "ogreFrames/frame.%04d.png";
  char fname[1024];

  int frame = 0;
  while(!readyToExit){

	world.timestep();
	for(auto& p : world.particles){
	  if(p.sceneNode == nullptr){
		p.cleanup = cleanupParticle;
		p.sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
		p.sceneNode->setScale(scaleFactor, scaleFactor, scaleFactor);
		p.entity = sceneManager->createEntity("sphere.mesh");
		p.sceneNode->attachObject(p.entity);
		auto material = Ogre::MaterialManager::getSingleton().create(
			"aMat",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		//find closest color:
		/*		auto closestCluster = std::min_element(
			p.clusters.begin(), p.clusters.end(),
			[&p,&world](int a, int b){
			  return (p.position - world.clusters[a].worldCom).squaredNorm() <
			  (p.position - world.clusters[b].worldCom).squaredNorm();
			});
		
			RGBColor rgb = HSLColor(2.0*acos(-1)*(*closestCluster%12)/12.0, 0.7, 0.7).to_rgb();*/
		material->getTechnique(0)->setDiffuse(p.color.r, p.color.g, p.color.b, 1);
		p.entity->setMaterial(material);
	  }
	  p.sceneNode->setPosition(p.position.x(), p.position.y(), p.position.z());
	}

	
	// Pump window messages for nice behaviour
	Ogre::WindowEventUtilities::messagePump();
	
	// Render a frame
	ogreRoot->renderOneFrame();
	if(dumpFrames){
	  sprintf(fname, fileBase.c_str(), frame);

	  window->writeContentsToFile(fname);
	}
	std::cout << "finished frame: " << frame << std::endl;
	frame++;
	
	if(window->isClosed()){
	  readyToExit = true;
	}
  }
  
  
  return 0;
}