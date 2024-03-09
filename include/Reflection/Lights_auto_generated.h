// !WARNING! : This file was generated automatically during build. DO NOT modify.
#pragma once

#include "Reflection.h"
#include <Lights.h>

const MemberMeta DirectionalLight_meta[] = {
	{ "shadowMapNearPlane", offsetof(DirectionalLight, shadowMapNearPlane), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "shadowMapFarPlane", offsetof(DirectionalLight, shadowMapFarPlane), zorya::VAR_REFL_TYPE::FLOAT }, 
};

BUILD_FOREACHFIELD(DirectionalLight)

const MemberMeta PointLight_meta[] = {
	{ "constant", offsetof(PointLight, constant), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "linear", offsetof(PointLight, linear), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "quadratic", offsetof(PointLight, quadratic), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "shadowMapNearPlane", offsetof(PointLight, shadowMapNearPlane), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "shadowMapFarPlane", offsetof(PointLight, shadowMapFarPlane), zorya::VAR_REFL_TYPE::FLOAT }, 
};

BUILD_FOREACHFIELD(PointLight)

const MemberMeta SpotLight_meta[] = {
	{ "cosCutoffAngle", offsetof(SpotLight, cosCutoffAngle), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "shadowMapNearPlane", offsetof(SpotLight, shadowMapNearPlane), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "shadowMapFarPlane", offsetof(SpotLight, shadowMapFarPlane), zorya::VAR_REFL_TYPE::FLOAT }, 
};

BUILD_FOREACHFIELD(SpotLight)

