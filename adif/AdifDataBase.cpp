#include "AdifDataBase.h"

AdifDataBase::AdifDataBase(std::string value) : m_rawValue(std::move(value)) {}

AdifDataBase::~AdifDataBase() = default;

std::string AdifDataBase::get() const { return m_rawValue; }