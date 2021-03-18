#include "DataComponent.h"

using namespace PaintsNow;

DataComponent::Property::Property(size_t s, size_t count) : size(s) {
	data.Resize(size * count);
}

DataComponent::DataComponent(size_t c) : maxObjectCount(c) {
	assert(c != 0);
	bitmap.resize((c + sizeof(size_t) - 1) / sizeof(size_t));
}

size_t DataComponent::SetProperty(const String& name, size_t size) {
	for (size_t i = 0; i < properties.size(); i++) {
		assert(properties[i]->name != name);
	}

	size_t index = properties.size();
	properties.emplace_back(TShared<Property>::From(new Property(size, maxObjectCount)));
	return index;
}

size_t DataComponent::GetProperty(const String& name) const {
	for (size_t i = 0; i < properties.size(); i++) {
		if (properties[i]->name == name) return i;
	}

	return ~(size_t)0;
}

uint8_t* DataComponent::GetPropertyData(size_t objectIndex, size_t propertyIndex) {
	assert(objectIndex < maxObjectCount);
	assert(IsObjectIndexValid(objectIndex));
	assert(propertyIndex < properties.size());

	Property& property = *properties[propertyIndex];
	return &property.data[objectIndex * property.size];
}

const uint8_t* DataComponent::GetPropertyData(size_t objectIndex, size_t propertyIndex) const {
	assert(objectIndex < maxObjectCount);
	assert(IsObjectIndexValid(objectIndex));
	assert(propertyIndex < properties.size());

	const Property& property = *properties[propertyIndex];
	return &property.data[objectIndex * property.size];
}

size_t DataComponent::GetPropertySize(size_t propertyIndex) const {
	assert(propertyIndex < properties.size());
	const Property& property = *properties[propertyIndex];
	return property.size;
}

bool DataComponent::IsObjectIndexValid(size_t objectIndex) const {
	const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&bitmap[0]);
	return (ptr[objectIndex >> 3] & (1 << (objectIndex & 7))) != 0;
}

size_t DataComponent::AllocateObject() {
	// find object
	for (size_t i = 0; i < bitmap.size(); i++) {
		size_t& mask = bitmap[i];
		if (mask != ~(size_t)0) {
			size_t s = Math::Alignment(mask + 1);
			size_t index = Math::Log2(s) + i * 8 * sizeof(size_t);
			if (index < maxObjectCount) {
				mask |= s;
				return index;
			}
		}
	}

	return ~(size_t)0;
}

void DataComponent::DeallocateObject(size_t objectIndex) {
	assert(IsObjectIndexValid(objectIndex));

	uint8_t* ptr = reinterpret_cast<uint8_t*>(&bitmap[0]);
	ptr[objectIndex >> 3] &= ~(1 << (objectIndex & 7));
}
