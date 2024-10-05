#include "../include/BasicRole.h"

::std::random_device my::BasicRole::m_device;
::std::mt19937 my::BasicRole::m_engine(m_device());
::std::uniform_real_distribution<float> my::BasicRole::m_distribution(0.0f, 1.0f);

::my::BasicRole::~BasicRole() {}
