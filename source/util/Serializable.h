#pragma once

class Serializable
{
public:
	virtual void Deserialize(const pugi::xml_node &node) = 0;
	virtual void Serialize(pugi::xml_node &parent_node) = 0;
};
