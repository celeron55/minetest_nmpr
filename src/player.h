/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include <irrlicht.h>

using namespace irr;
typedef core::vector3df v3f;
typedef core::vector3d<s16> v3s16;

class Map;

/*
	TODO: Make this a scene::ISceneNode
*/

class Player : public scene::ISceneNode
{
public:
	Player(bool is_local);
	
	Player(
		bool is_local,
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id);
	
	~Player();

	void move(f32 dtime, Map &map);

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode()
	{
		if (IsVisible)
			SceneManager->registerNodeForRendering(this);

		ISceneNode::OnRegisterSceneNode();
	}

	virtual void render()
	{
		// Do nothing
	}
	
	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{
		return m_box;
	}

	v3f getPosition()
	{
		return m_position;
	}

	void setPosition(v3f position)
	{
		m_position = position;
		updateSceneNodePosition();
	}

	void updateSceneNodePosition()
	{
		ISceneNode::setPosition(m_position);
	}

	bool isLocal()
	{
		return m_is_local;
	}

	v3f speed;
	bool touching_ground;
	u16 peer_id;
	float timeout_counter;

private:
	bool m_is_local;
	v3f m_position;
	//scene::ISceneNode* m_bill;
	scene::IBillboardSceneNode* m_bill;
	
	core::aabbox3d<f32> m_box;
};

#endif

