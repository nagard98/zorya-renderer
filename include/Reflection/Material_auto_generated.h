// !WARNING! : This file was generated automatically during build. DO NOT modify.
#pragma once

#include "Reflection.h"
#include <Material.h>

const MemberMeta MaterialDesc_meta[] = {
	{ "albedoPath", offsetof(MaterialDesc, albedoPath), zorya::VAR_REFL_TYPE::WCHAR }, 
	{ "baseColor", offsetof(MaterialDesc, baseColor), zorya::VAR_REFL_TYPE::XMFLOAT4 }, 
	{ "subsurfaceAlbedo", offsetof(MaterialDesc, subsurfaceAlbedo), zorya::VAR_REFL_TYPE::XMFLOAT4 }, 
	{ "meanFreePathColor", offsetof(MaterialDesc, meanFreePathColor), zorya::VAR_REFL_TYPE::XMFLOAT4 }, 
	{ "meanFreePathDistance", offsetof(MaterialDesc, meanFreePathDistance), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "scale", offsetof(MaterialDesc, scale), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "smoothnessValue", offsetof(MaterialDesc, smoothnessValue), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "metalnessValue", offsetof(MaterialDesc, metalnessValue), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "normalPath", offsetof(MaterialDesc, normalPath), zorya::VAR_REFL_TYPE::WCHAR }, 
};

BUILD_FOREACHFIELD(MaterialDesc)

const MemberMeta MaterialParams_meta[] = {
	{ "meanFreePathColor", offsetof(MaterialParams, meanFreePathColor), zorya::VAR_REFL_TYPE::XMFLOAT4 }, 
	{ "roughness", offsetof(MaterialParams, roughness), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "metalness", offsetof(MaterialParams, metalness), zorya::VAR_REFL_TYPE::FLOAT }, 
	{ "meanFreePathDist", offsetof(MaterialParams, meanFreePathDist), zorya::VAR_REFL_TYPE::FLOAT }, 
};

BUILD_FOREACHFIELD(MaterialParams)

