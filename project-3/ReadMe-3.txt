  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  
  //  --------------------------------------------- Assignemnt #3 ReadMe File --------------------------------------------------------------------
  //  Name   		  : Sumukh Lagadamane Shivashankara

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
************************************************************************************************************************************************
  USAGE:
  
  1.For viewing the still ray traced images
	a. DISPLAY_MODE
	 assign3 <SceneFileName.txt> 
	b. JPEG_MODE
	 assign3 <SceneFileName.txt> <StillImageFileName.jpg>
	NOTE: the output is saved as StillFileName.jpg
	 
  2.For viewing the effect of Motion Blur
    a. DISPLAY_MODE
	 assign3 <SceneFileName.txt> 1 0 0
	b. JPEG_MODE
	 assign3 <SceneFileName.txt> 1 0 1
	NOTE: Output images are prefixed with "B_" and saved. ex: B_001.jpg. 
	NOTE: The Best results for this effect can be observed for table.txt

  3.For viewing the animation based on lighting changes
    a. DISPLAY_MODE
	 assign3 <SceneFileName.txt> 0 1 0
	b. JPEG_MODE
	 assign3 <SceneFileName.txt> 0 1 1
	NOTE: Output images are prefixed with "A_" and saved. ex: A_001.jpg
	NOTE: The Best results for this effect can be observed for sphere.txt
	 
  ************************************************************************************************************************************************
  MANDATORY FEATURES :

  FEATURE:                                 STATUS: Finish? (Yes/No)
  -------------------------------------    -------------------------
  1) Ray tracing triangles                  Yes

  2) Ray tracing sphere                     Yes

  3) Triangle Phong Shading                 Yes

  4) Sphere Phong Shading                   Yes

  5) Shadows rays                           Yes

  6) Still images                           Yes
   
  7) Extra Credit
  		a. Animations by changing the Lighting
			 - The lights in the scene are constantly moved to create animations
			 
		b. Motion Blur effect
			 - This effect was implemented using ray tracing and the accumulation buffer
			 - The accumulation buffer accumulates the color for a set of frames and is then returned to the display thus poducing the effect of motion blur 

  ************************************************************************************************************************************************
  SUBMITTED IMAGES:

	1. Still images with ray tracing for various scenes with fov as 60 and 90 
	2. light movement animation frames: A_001.jpg..  
	3. motion blur animation frames   : B_001.jpg..  
	
  ************************************************************************************************************************************************
	
	