/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __UTIL_MMD_HPP__
#define __UTIL_MMD_HPP__

#include <array>
#include <cinttypes>
#include <vector>
#include <memory>
#include <mathutil/umath.h>

class VFilePtrInternal;
namespace mmd
{
	namespace pmx
	{
		enum class DrawingMode : char
		{
			NoCull = 0,
			GroundShadow = 1,
			DrawShadow = 2,
			ReceiveShadow = 3,
			HasEdge = 4,
			VertexColor = 5,
			PointDrawing = 6,
			LineDrawing = 7
		};

		enum class BoneFlag : uint16_t
		{
			None = 0,
			IndexedTailPosition = 1,
			Rotatable = IndexedTailPosition<<1,
			Translatable = Rotatable<<1,
			IsVisible = Translatable<<1,
			Enabled = IsVisible<<1,
			IK = Enabled<<1,
			InheritRotation = IK<<3,
			InheritTranslation = InheritRotation<<1,
			FixedAxis = InheritTranslation<<1,
			LocalCoordinate = FixedAxis<<1,
			PhysicsAfterDeform = LocalCoordinate<<1,
			ExternalParentDeform = PhysicsAfterDeform<<1
		};
		REGISTER_BASIC_BITWISE_OPERATORS(BoneFlag);

		struct VertexData
		{
			std::array<float,3> position;
			std::array<float,3> normal;
			std::array<float,2> uv;
			std::array<int32_t,4> boneIds = {-1,-1,-1,-1};
			std::array<float,4> boneWeights = {0.f,0.f,0.f,0.f};
		};

		struct MaterialData
		{
			std::string name;
			std::array<float,4> diffuseColor;
			std::array<float,3> specularColor;
			float specularity = 0.f;
			std::array<float,3> ambientColor;
			DrawingMode drawingMode = DrawingMode::NoCull;
			std::array<float,4> edgeColor;
			float edgeSize = 0.f;
			int32_t textureIndex = -1;
			int32_t sphereIndex = -1;
			char sphereMode = 0;
			char toonFlag = 0;
			int32_t toonIndex = -1;
			std::string memo;
			int32_t faceCount = -1;
		};

		struct Bone
		{
			std::string name;
			std::array<float,3> position;
			int32_t parentBoneIdx = -1;
			int32_t layer = -1;
			BoneFlag flags = BoneFlag::None;
		};

		struct ModelData
		{
			float version = 0.f;
			std::string characterName;
			std::string comment;
			std::vector<VertexData> vertices;
			std::vector<uint16_t> faces;
			std::vector<std::string> textures;
			std::vector<MaterialData> materials;
			std::vector<Bone> bones;
		};

		std::shared_ptr<ModelData> load(const std::string &path);
		std::shared_ptr<ModelData> load(std::shared_ptr<VFilePtrInternal> &f);
	};
};

#endif
