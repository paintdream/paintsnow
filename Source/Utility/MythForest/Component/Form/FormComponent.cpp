#include "FormComponent.h"

using namespace PaintsNow;

FormComponent::FormComponent(const String& n) : name(n) {}

TShared<SharedTiny> FormComponent::GetCookie(void* key) const {
	std::vector<std::key_value<void*, TShared<SharedTiny> > >::const_iterator it = std::binary_find(cookies.begin(), cookies.end(), key);
	return it == cookies.end() ? TShared<SharedTiny>(nullptr) : it->second;
}

void FormComponent::SetCookie(void* key, const TShared<SharedTiny>& tiny) {
	if (tiny) {
		std::binary_insert(cookies, std::make_key_value(key, tiny));
	} else {
		std::binary_erase(cookies, key);
	}
}

void FormComponent::ClearCookies() {
	cookies.clear();
}

void FormComponent::SetName(const String& n) {
	name = n;
}

void FormComponent::SetName(rvalue<String> n) {
	name = std::move(n);
}

const String& FormComponent::GetName() const {
	return name;
}

std::vector<String>& FormComponent::GetValues() {
	return values;
}

const std::vector<String>& FormComponent::GetValues() const {
	return values;
}


