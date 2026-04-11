#include "AdifDataBase.h"

AdifDataBase::AdifDataBase(std::string value) : m_rawValue(std::move(value)) {}

AdifDataBase::~AdifDataBase() = default;

auto AdifDataBase::get() const -> std::string { return m_rawValue; }