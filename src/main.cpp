/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "util_mmd.hpp"
#include <fsys/filesystem.h>
#include <utf8.h>

#pragma comment(lib,"vfilesystem.lib")
#pragma comment(lib,"mathutil.lib")

#ifdef MMD_TEST
#include <iostream>
#pragma optimize("",off)
#endif

namespace mmd
{
	namespace pmx
	{
		enum class TextEncoding : char
		{
			UTF16 = 0,
			UTF8
		};

		enum class WeightType : char
		{
			BDEF1 = 0,
			BDEF2 = 1,
			BDEF4 = 2,
			SDEF = 3,
			QDEF = 4
		};

		enum class IndexType : char
		{
			Byte = 1,
			Short = 2,
			Int = 4
		};

		enum class SoftBodyFlag : char
		{
			None = 0,
			BLink = 1,
			ClusterCreation = BLink<<1,
			LinkCrossing = ClusterCreation<<1
		};
		REGISTER_BASIC_BITWISE_OPERATORS(SoftBodyFlag);

		static std::string read_text(VFilePtr &f,TextEncoding encoding);
		template<typename T0,typename T1,typename T2>
			int32_t read_index(VFilePtr &f,IndexType type);
		static int32_t read_index(VFilePtr &f,IndexType type);
		static int32_t read_vertex_index(VFilePtr &f,IndexType type);
	};
};

std::string mmd::pmx::read_text(VFilePtr &f,TextEncoding encoding)
{
	auto len = f->Read<int32_t>();
	switch(encoding)
	{
		case TextEncoding::UTF8:
		{
			std::vector<int8_t> data(len);
			f->Read(data.data(),len);
			return std::string(data.begin(),data.end());
		}
		case TextEncoding::UTF16:
		{
			std::vector<int16_t> data(len /2 +((len %2) == 0 ? 0 : 1));
			f->Read(data.data(),len);

			std::vector<int8_t> utf8Data;
			utf8::utf16to8(data.begin(),data.end(),std::back_inserter(utf8Data));
			return std::string(utf8Data.begin(),utf8Data.end());
		}
	}
	return "";
}
template<typename T0,typename T1,typename T2>
	int32_t mmd::pmx::read_index(VFilePtr &f,IndexType type)
{
	switch(type)
	{
		case IndexType::Byte:
			return f->Read<T0>();
		case IndexType::Short:
			return f->Read<T1>();
		case IndexType::Int:
			return f->Read<T2>();
	}
	throw std::logic_error("Invalid index type");
}
int32_t mmd::pmx::read_index(VFilePtr &f,IndexType type) {return read_index<int8_t,int16_t,int32_t>(f,type);}
int32_t mmd::pmx::read_vertex_index(VFilePtr &f,IndexType type) {return read_index<uint8_t,uint16_t,int32_t>(f,type);}
std::shared_ptr<mmd::pmx::ModelData> mmd::pmx::load(VFilePtr &f)
{
	auto signature = f->Read<std::array<char,4>>();
	if(signature.at(0) != 'P' || signature.at(1) != 'M' || signature.at(2) != 'X' || signature.at(3) != ' ')
		return nullptr;
	auto version = f->Read<float>();
	if(version != 2.f)
		return nullptr;
	auto len = f->Read<char>();
	auto textEncoding = f->Read<TextEncoding>();
	auto appendixUvCount = f->Read<char>();
	auto vertexIndexSize = f->Read<IndexType>();
	auto textureIndexSize = f->Read<IndexType>();
	auto materialIndexSize = f->Read<IndexType>();
	auto boneIndexSize = f->Read<IndexType>();
	auto morphIndexSize = f->Read<IndexType>();
	auto rigidBodyIndexSize = f->Read<IndexType>();

	auto mdlData = std::make_shared<ModelData>();
	mdlData->version = version;
	auto characterName = read_text(f,textEncoding);
	mdlData->characterName = read_text(f,textEncoding);
	auto comment = read_text(f,textEncoding);
	mdlData->comment = read_text(f,textEncoding);

	auto vertexCount = f->Read<int32_t>();
	mdlData->vertices.reserve(vertexCount);
	for(auto i=decltype(vertexCount){0};i<vertexCount;++i)
	{
		mdlData->vertices.push_back({});
		auto &v = mdlData->vertices.back();
		v.position = f->Read<std::array<float,3>>();
		v.normal = f->Read<std::array<float,3>>();
		v.uv = f->Read<std::array<float,2>>();
		std::vector<float> appendixUv(appendixUvCount);
		f->Read(appendixUv.data(),appendixUv.size() *sizeof(appendixUv.front()));
		auto weightType = f->Read<WeightType>();
		switch(weightType)
		{
			case WeightType::BDEF1:
			{
				v.boneIds.at(0) = read_index(f,boneIndexSize);
				v.boneWeights.at(0) = 1.f;
				break;
			}
			case WeightType::BDEF2:
			{
				v.boneIds.at(0) = read_index(f,boneIndexSize);
				v.boneIds.at(1) = read_index(f,boneIndexSize);
				auto &weight0 = v.boneWeights.at(0) = f->Read<float>();
				v.boneWeights.at(1) = 1.f -weight0;
				break;
			}
			case WeightType::BDEF4:
			case WeightType::QDEF:
			{
				for(uint8_t i=0;i<4;++i)
					v.boneIds.at(i) = read_index(f,boneIndexSize);

				for(uint8_t i=0;i<4;++i)
					v.boneWeights.at(i) = f->Read<float>();
				break;
			}
			case WeightType::SDEF:
			{
				v.boneIds.at(0) = read_index(f,boneIndexSize);
				v.boneIds.at(1) = read_index(f,boneIndexSize);
				auto &weight0 = v.boneWeights.at(0) = f->Read<float>();
				v.boneWeights.at(1) = 1.f -weight0;
				auto c = f->Read<std::array<float,3>>();
				auto r0 = f->Read<std::array<float,3>>();
				auto r1 = f->Read<std::array<float,3>>();

				break;
			}
			default:
				throw std::runtime_error("Invalid weight type: " +std::to_string(umath::to_integral(weightType)));
		}
		auto edgeScale = f->Read<float>();
	}

	auto numFaces = f->Read<int32_t>();
	mdlData->faces.reserve(numFaces);
	for(auto i=decltype(numFaces){0};i<numFaces;++i)
	{
		auto vertIdx = read_vertex_index(f,vertexIndexSize);
		mdlData->faces.push_back(vertIdx);
	}

	auto numTextures = f->Read<int32_t>();
	mdlData->textures.reserve(numTextures);
	for(auto i=decltype(numTextures){0};i<numTextures;++i)
	{
		auto fileName = read_text(f,textEncoding);
		mdlData->textures.push_back(fileName);
	}

	auto numMaterials = f->Read<int32_t>();
	mdlData->materials.reserve(numMaterials);
	for(auto i=decltype(numMaterials){0};i<numMaterials;++i)
	{
		mdlData->materials.push_back({});
		auto &mat = mdlData->materials.back();
		auto name = read_text(f,textEncoding);
		mat.name = read_text(f,textEncoding);
		mat.diffuseColor = f->Read<std::array<float,4>>();
		mat.specularColor = f->Read<std::array<float,3>>();
		mat.specularity = f->Read<float>();
		mat.ambientColor = f->Read<std::array<float,3>>();
		mat.drawingMode = f->Read<DrawingMode>();
		mat.edgeColor = f->Read<std::array<float,4>>();
		mat.edgeSize = f->Read<float>();
		mat.textureIndex = read_index(f,textureIndexSize);
		mat.sphereIndex = read_index(f,textureIndexSize);
		mat.sphereMode = f->Read<char>();
		mat.toonFlag = f->Read<char>();
		mat.toonIndex = (mat.toonFlag == 0) ? read_index(f,textureIndexSize) : f->Read<int8_t>();
		mat.memo = read_text(f,textEncoding);
		mat.faceCount = f->Read<int32_t>();
	}

	auto numBones = f->Read<int32_t>();
	mdlData->bones.reserve(numBones);
	for(auto i=decltype(numBones){0};i<numBones;++i)
	{
		mdlData->bones.push_back({});
		auto &bone = mdlData->bones.back();
		auto name = read_text(f,textEncoding);
		bone.name = read_text(f,textEncoding);
		bone.position = f->Read<std::array<float,3>>();
		bone.parentBoneIdx = read_index(f,boneIndexSize);
		bone.layer = f->Read<int32_t>();
		bone.flags = f->Read<BoneFlag>();
		if((bone.flags &BoneFlag::IndexedTailPosition) != BoneFlag::None)
		{
			auto tailPos = read_index(f,boneIndexSize);

		}
		else
		{
			auto tailPos = f->Read<std::array<float,3>>();

		}
		if((bone.flags &(BoneFlag::InheritRotation | BoneFlag::InheritTranslation)) != BoneFlag::None)
		{
			auto parentIndex = read_index(f,boneIndexSize);
			auto parentInfluence = f->Read<float>();

		}
		if((bone.flags &BoneFlag::FixedAxis) != BoneFlag::None)
		{
			auto axisDirection = f->Read<std::array<float,3>>();

		}
		if((bone.flags &BoneFlag::LocalCoordinate) != BoneFlag::None)
		{
			auto xVec = f->Read<std::array<float,3>>();
			auto yVec = f->Read<std::array<float,3>>();

		}
		if((bone.flags &BoneFlag::ExternalParentDeform) != BoneFlag::None)
		{
			auto parentIndex = read_index(f,boneIndexSize);

		}
		if((bone.flags &BoneFlag::IK) != BoneFlag::None)
		{
			auto targetIndex = read_index(f,boneIndexSize);
			auto loopCount = f->Read<int32_t>();
			auto limitRadian = f->Read<float>();
			auto linkCount = f->Read<int32_t>();
			for(auto i=decltype(linkCount){0};i<linkCount;++i)
			{
				auto boneIndex = read_index(f,boneIndexSize);
				auto hasLimits = f->Read<int8_t>();
				if(hasLimits == 1)
				{
					auto min = f->Read<std::array<float,3>>();
					auto max = f->Read<std::array<float,3>>();

				}
			}
		}
	}

	/*auto numMorphs = f->Read<int32_t>();
	for(auto i=decltype(numMorphs){0};i<numMorphs;++i)
	{
		auto morphNameLocal = read_text(f,textEncoding);
		auto morphNameGlobal = read_text(f,textEncoding);
		auto panelType = f->Read<int8_t>();
		auto morphType = f->Read<int8_t>();
		auto offsetCount = f->Read<int32_t>();
		if(morphType == 1)
		{
			for(auto i=decltype(offsetCount){0};i<offsetCount;++i)
			{
				auto vertIdx = read_vertex_index(f,morphIndexSize);
				std::cout<<vertIdx<<std::endl;
			}
		}
		// ???
		//auto offset
	}

	auto numDisplayFrames = f->Read<int32_t>();
	for(auto i=decltype(numDisplayFrames){0};i<numDisplayFrames;++i)
	{
		auto nameLocal = read_text(f,textEncoding);
		auto nameGlobal = read_text(f,textEncoding);
		auto specialFlag = f->Read<int8_t>();
		auto numFrames = f->Read<int32_t>();
		for(auto i=decltype(numFrames){0};i<numFrames;++i)
			;
	}

	auto numRigidBodies = f->Read<int32_t>();
	for(auto i=decltype(numRigidBodies){0};i<numRigidBodies;++i)
	{
		auto nameLocal = read_text(f,textEncoding);
		auto nameGlobal = read_text(f,textEncoding);
		auto relatedBoneIndex = read_index(f,boneIndexSize);
		auto groupId = f->Read<int8_t>();
		auto nonCollisionGroup = f->Read<int16_t>();
		auto shapeType = f->Read<int8_t>();
		auto shapeSize = f->Read<std::array<float,3>>();
		auto shapePos = f->Read<std::array<float,3>>();
		auto shapeRot = f->Read<std::array<float,3>>();
		auto mass = f->Read<float>();
		auto moveAttenuation = f->Read<float>();
		auto rotDamping = f->Read<float>();
		auto repulsion = f->Read<float>();
		auto frictionForce = f->Read<float>();
		auto physicsMode = f->Read<int8_t>();

	}

	auto numJoints = f->Read<int32_t>();
	for(auto i=decltype(numJoints){0};i<numJoints;++i)
	{
		auto nameLocal = read_text(f,textEncoding);
		auto nameGlobal = read_text(f,textEncoding);
		auto jointType = f->Read<int8_t>();
		auto rigidBodyIndexA = read_index(f,rigidBodyIndexSize);
		auto rigidBodyIndexB = read_index(f,rigidBodyIndexSize);
		auto pos = f->Read<std::array<float,3>>();
		auto rot = f->Read<std::array<float,3>>();
		auto minPos = f->Read<std::array<float,3>>();
		auto maxPos = f->Read<std::array<float,3>>();
		auto rotMin = f->Read<std::array<float,3>>();
		auto rotMax = f->Read<std::array<float,3>>();
		auto posSpring = f->Read<std::array<float,3>>();
		auto rotSpring = f->Read<std::array<float,3>>();

	}

	if(version >= 2.1f)
	{
		auto numSoftBodies = f->Read<int32_t>();
		for(auto i=decltype(numSoftBodies){0};i<numSoftBodies;++i)
		{
			auto nameLocal = read_text(f,textEncoding);
			auto nameGlobal = read_text(f,textEncoding);
			auto shapeType = f->Read<int8_t>();
			auto matIdx = read_index(f,materialIndexSize);
			auto groupId = f->Read<int32_t>();
			auto noCollisionMask = f->Read<int16_t>();
			auto flags = f->Read<SoftBodyFlag>();
			auto bLinkCreateDistance = f->Read<int32_t>();
			auto numberOfClusters = f->Read<int32_t>();
			auto totalMass = f->Read<float>();
			auto collisionMargin = f->Read<float>();
			auto aerodynamicsModel = f->Read<int32_t>();
			auto configVCF = f->Read<float>();
			auto configDP = f->Read<float>();
			auto configDG = f->Read<float>();
			auto configLF = f->Read<float>();
			auto configPR = f->Read<float>();
			auto configVC = f->Read<float>();
			auto configDF = f->Read<float>();
			auto configMT = f->Read<float>();
			auto configCHR = f->Read<float>();
			auto configKHR = f->Read<float>();
			auto configSHR = f->Read<float>();
			auto configAHR = f->Read<float>();
			auto srhrCl = f->Read<float>();
			auto skhrCl = f->Read<float>();
			auto sshrCl = f->Read<float>();
			auto srSpltCl = f->Read<float>();
			auto skSpltCl = f->Read<float>();
			auto ssSpltCl = f->Read<float>();
			auto vIt = f->Read<int32_t>();
			auto pIt = f->Read<int32_t>();
			auto dIt = f->Read<int32_t>();
			auto cIt = f->Read<int32_t>();
			auto lst = f->Read<int32_t>();
			auto ast = f->Read<int32_t>();
			auto vst = f->Read<int32_t>();
			auto anchorRigidBodyCount = f->Read<int32_t>();
			// TODO
			auto vertexPinCount = f->Read<int32_t>();
			// TODO
		}
	}*/

	return mdlData;
}

std::shared_ptr<mmd::pmx::ModelData> mmd::pmx::load(const std::string &path)
{
	VFilePtr f = FileManager::OpenSystemFile(path.c_str(),"rb");
	if(f == nullptr)
		return nullptr;
	return load(f);
}
#ifdef MMD_TEST
int main(int argc,char *argv[])
{
	auto mdlData = mmd::pmx::load("mdl.pmx");

	for(;;);
	return EXIT_SUCCESS;
}
#pragma optimize("",on)
#endif
