#ifndef BCTEXTURE_H
#define BCTEXTURE_H


#include "vframe.inc"


// Container for texture objects

class BC_Texture
{
public:
	BC_Texture(int w, int h, int colormodel);
	~BC_Texture();

	friend class VFrame;

// Create a new texture if *texture if 0 
// or update the existing texture if *texture is 
// nonzero.  The created texture object is stored in *texture.
// The texture parameters are stored in the texture manager.
// The user must delete *texture when finished with it.
// The texture is bound to the current texture unit and enabled.
// Must be called from a synchronous opengl thread after enable_opengl.
	static void new_texture(BC_Texture **texture,
		int w, 
		int h, 
		int colormodel);

// Bind the frame's texture to GL_TEXTURE_2D and enable it.
// If a texture_unit is supplied, the texture unit is made active
// and the commands are run in the right sequence to 
// initialize it to our preferred specifications.
// The texture unit initialization requires the texture to be bound.
	void bind(int texture_unit = -1);

// Calculate the power of 2 size for allocating textures
	static int calculate_texture_size(int w, int *max = 0);
	int get_texture_id();
	int get_texture_w();
	int get_texture_h();
	int get_texture_components();
	int get_window_id();

private:
	void clear_objects();

// creates a new texture or updates an existing texture to work with the
// current window.
	void create_texture(int w, int h, int colormodel);


	int window_id;
	int texture_id;
	int texture_w;
	int texture_h;
	int texture_components;
	int colormodel;
	int w;
	int h;
};


#endif
