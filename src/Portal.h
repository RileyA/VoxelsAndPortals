#ifndef Portal_H
#define Portal_H

#include "Common.h"
#include "Oryx.h"
#include "OryxObject.h"
#include "Oryx3DMath.h"
#include "ChunkOptions.h"
#include "ChunkCoords.h"
#include "OgreSubsystem/OgreSubsystem.h"

class BasicChunk;

/** A Portal, a la the game Portal; imagine that. */
class Portal : public Oryx::Object
{
public:

	/** Constructor
	 *		@param pos The positon for the portal
	 *		@param out The facing direction 
	 *		@param up The 'up' direction
	 *		@param blue Is this the blue portal (orange otherwise) */
	Portal(Vector3 pos, BlockDirection out, BlockDirection up, bool blue);

	/** Destructor */
	virtual ~Portal();

	/** Updates camera positioning
	 *		@param delta Delta time */
	void update(Real delta);

	/** Bind this portal to another
	 *		@param p The other portal
	 *		@remarks This is one-way, so you'll have to call it on both
	 *			to get a 2-way connection (and one-way connections 
	 *			don't actually do anything yet...) */
	void setSibling(Portal* p);

	/** Toggle visibility */
	void setVisible(bool v);

	/** Set position */
	void setPosition(Vector3 p);

	/** Set direction based on a direction and an up vector */
	void setDirection(Vector3 d, Vector3 up);

	/** Set direction based on voxel chunk directions */
	void setDirection(BlockDirection out, BlockDirection up);

	/** Transform camera position for snazzy portal rendering */
	void recurse();

	/** Get the portal camera */
	Camera* getCamera();

	// hackity hack...
	BasicChunk* chunks[2];
	byte lightVals[2];
	ChunkCoords cc[2];
	bool placed;
	Vector3 upv;
	Vector3 out;

//protected: pssshhhh encapsulation?

	// Hold onto a pointer to the gfx system for convenience
	OgreSubsystem* mOgre;

	// The camera that will be rendered through
	Camera* mCamera;

	// The node that the camera is attached to, yawed 180 degrees
	SceneNode* mNode;

	// The portal mesh (used only for writing to the stencil buffer, 
	// will be totally invisible otherwise)
	Mesh* mMesh;

	// The border around the portal, purely decorative
	Mesh* mBorder;

	// The portal that this is connected to (if any)
	Portal* mSibling;

	CollisionObject* mCollision;

	// Position and direction of portal
	Vector3 mDirection;
	Vector3 mPosition;

	// sort of a hack, but distinguishes this portal from it's mate
	// for rendering and other purposes
	bool mBlue;

};

#endif
