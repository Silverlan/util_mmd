/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <array>
#include <cinttypes>
#include <vector>
#include <memory>
#include <string>
#include <mathutil/umath.h>
#include <mathutil/umat.h>

namespace ufile {
	struct IFile;
};

export module util_mmd;

export namespace mmd {
	namespace pmx {
		enum class DrawingMode : uint8_t { NoCull = 1, GroundShadow = NoCull << 1u, DrawShadow = GroundShadow << 1u, ReceiveShadow = DrawShadow << 1u, HasEdge = ReceiveShadow << 1u, VertexColor = HasEdge << 1u, PointDrawing = VertexColor << 1u, LineDrawing = PointDrawing << 1u };
		REGISTER_BASIC_BITWISE_OPERATORS(DrawingMode);

		enum class BoneFlag : uint16_t {
			None = 0,
			IndexedTailPosition = 1,
			Rotatable = IndexedTailPosition << 1,
			Translatable = Rotatable << 1,
			IsVisible = Translatable << 1,
			Enabled = IsVisible << 1,
			IK = Enabled << 1,
			InheritRotation = IK << 3,
			InheritTranslation = InheritRotation << 1,
			FixedAxis = InheritTranslation << 1,
			LocalCoordinate = FixedAxis << 1,
			PhysicsAfterDeform = LocalCoordinate << 1,
			ExternalParentDeform = PhysicsAfterDeform << 1
		};
		REGISTER_BASIC_BITWISE_OPERATORS(BoneFlag);

		enum class MorphType : int8_t { Group = 0, Vertex, Bone, Uv, Uva1, Uva2, Uva3, Uva4, Material, Flip, Impulse };

		struct VertexData {
			std::array<float, 3> position;
			std::array<float, 3> normal;
			std::array<float, 2> uv;
			std::array<int32_t, 4> boneIds = {-1, -1, -1, -1};
			std::array<float, 4> boneWeights = {0.f, 0.f, 0.f, 0.f};
		};

		struct MaterialData {
			std::string name;
			std::array<float, 4> diffuseColor;
			std::array<float, 3> specularColor;
			float specularity = 0.f;
			std::array<float, 3> ambientColor;
			DrawingMode drawingMode = DrawingMode::NoCull;
			std::array<float, 4> edgeColor;
			float edgeSize = 0.f;
			int32_t textureIndex = -1;
			int32_t sphereIndex = -1;
			char sphereMode = 0;
			char toonFlag = 0;
			int32_t toonIndex = -1;
			std::string memo;
			int32_t faceCount = -1;
		};

		struct Bone {
			std::string nameJp;
			std::string name;
			Vector3 position;
			int32_t parentBoneIdx = -1;
			int32_t layer = -1;
			BoneFlag flags = BoneFlag::None;
			Mat3 rotation = umat::identity();
		};

		struct BaseMorph {};

#pragma pack(push, 1)
		struct GroupMorph : public BaseMorph {
			int32_t index;
			float ratio;
		};

		struct VertexMorph : public BaseMorph {
			int32_t index;
			Vector3 offset;
		};

		struct BoneMorph : public BaseMorph {
			int32_t index;
			Vector3 translation;
			Vector4 rotation;
		};

		struct UvMorph : public BaseMorph {
			int32_t index;
			Vector4 offset;
		};

		struct MaterialMorph : public BaseMorph {
			int32_t index;
			uint8_t type;
			Vector4 diffuse;
			Vector3 specular;
			float shininess;
			Vector3 ambient;
			Vector4 edgeColor;
			float edgeSize;
			Vector4 tex;
			Vector4 sphere;
			Vector4 toon;
		};

		struct ImpulseMorph : public BaseMorph {
			int32_t index;
			uint8_t local;
			Vector3 velocity;
			Vector3 torque;
		};
#pragma pack(pop)

		struct Morph {
			~Morph();
			std::string nameLocal;
			std::string nameGlobal;
			int8_t panelType;
			MorphType type;
			int32_t count;

			BaseMorph *morphs;
		};

		struct ModelData {
			float version = 0.f;
			std::string characterName;
			std::string comment;
			std::vector<VertexData> vertices;
			std::vector<uint32_t> faces;
			std::vector<std::string> textures;
			std::vector<MaterialData> materials;
			std::vector<Bone> bones;
			std::vector<std::unique_ptr<Morph>> morphs;
		};

		std::shared_ptr<ModelData> load(const std::string &path);
		std::shared_ptr<ModelData> load(ufile::IFile &f);
	};

	namespace vmd {
#pragma pack(push, 1)
		struct Keyframe {
			std::array<char, 15> boneName;
			uint32_t frameIndex;
			std::array<float, 3> position;
			std::array<float, 4> rotation;
			std::array<uint8_t, 64> interpolation;
		};
		struct Morph {
			std::array<char, 15> morphName;
			uint32_t frameIndex;
			float weight;
		};
		struct Camera {
			uint32_t frameIndex;
			float negDistance;
			std::array<float, 3> position;
			std::array<float, 3> angles;
			std::array<uint8_t, 24> interpolation;
			uint32_t viewingngle;
			uint8_t perspective;
		};
		struct Light {
			uint32_t frameIndex;
			std::array<float, 3> color;
			std::array<float, 3> position;
		};
#pragma pack(pop)
		struct AnimationData {
			std::string modelName;
			std::vector<Keyframe> keyframes;
			std::vector<Morph> morphs;
			std::vector<Camera> cameras;
			std::vector<Light> lights;
		};
		std::shared_ptr<AnimationData> load(const std::string &path);
		std::shared_ptr<AnimationData> load(ufile::IFile &f);
	};
};
