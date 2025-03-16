#ifndef JIMENEZ_SEPARABLE_H_
#define JIMENEZ_SEPARABLE_H_

#include <vector>
#include <DirectXMath.h>
#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>

#include <Platform.h>

namespace dx = DirectX;

namespace zorya
{
	struct Kernel_Sample
	{
		float r, g, b, x, y;
	};

	float num_samples = 9;
	std::vector<dx::XMFLOAT4> kernel;

	void load_kernel_file(std::string fileName, std::vector<float>& data)
	{
		//data.clear();

		//std::string folder(kernelFolder.begin(), kernelFolder.end()); // dirty conversion
		std::string path = fileName;//folder + fileName;

		bool binary = false;
		if (fileName.compare(fileName.size() - 3, 3, ".bn") == 0)
			binary = true;

		std::ifstream i;
		std::ios_base::openmode om;

		if (binary) om = std::ios_base::in | std::ios_base::binary;
		else om = std::ios_base::in;

		i.open(path, om);

		if (!i.good())
		{
			i.close();
			i.open("../" + path, om);
		}

		if (binary)
		{
			data.clear();

			// read float count
			char sv[4];
			i.read(sv, 4);
			int fc = (int)(floor(*((float*)sv)));

			data.resize(fc);
			i.read(reinterpret_cast<char*>(&data[0]), fc * 4);
		} else
		{
			float v;

			while (i >> v)
			{
				data.push_back(v);

				int next = i.peek();
				switch (next)
				{
				case ',': i.ignore(1); break;
				case ' ': i.ignore(1); break;
				}
			}
		}

		i.close();
	}

	void calculate_offset(float _range, float _exponent, int _offsetCount, std::vector<float>& _offsets)
	{
		// Calculate the offsets:
		float step = 2.0f * _range / (_offsetCount - 1);
		for (int i = 0; i < _offsetCount; i++)
		{
			float o = -_range + float(i) * step;
			float sign = o < 0.0f ? -1.0f : 1.0f;
			float ofs = _range * sign * abs(pow(o, _exponent)) / pow(_range, _exponent);
			_offsets.push_back(ofs);
		}
	}

	void calculate_areas(std::vector<float>& _offsets, std::vector<float>& _areas)
	{
		int size = _offsets.size();

		for (int i = 0; i < size; i++)
		{
			float w0 = i > 0 ? abs(_offsets[i] - _offsets[i - 1]) : 0.0f;
			float w1 = i < size - 1 ? abs(_offsets[i] - _offsets[i + 1]) : 0.0f;
			float area = (w0 + w1) / 2.0f;
			_areas.push_back(area);
		}
	}

	dx::XMFLOAT3 lin_interpol_1d(std::vector<Kernel_Sample> _kernelData, float _x)
	{
		// naive, a lot to improve here

		if (_kernelData.size() < 1) throw "_kernelData empty";

		unsigned int i = 0;
		while (i < _kernelData.size())
		{
			if (_x > _kernelData[i].x) i++;
			else break;
		}

		dx::XMFLOAT3 v;

		if (i < 1)
		{
			v.x = _kernelData[0].r;
			v.y = _kernelData[0].g;
			v.z = _kernelData[0].b;
		} else if (i > _kernelData.size() - 1)
		{
			v.x = _kernelData[_kernelData.size() - 1].r;
			v.y = _kernelData[_kernelData.size() - 1].g;
			v.z = _kernelData[_kernelData.size() - 1].b;
		} else
		{
			Kernel_Sample b = _kernelData[i];
			Kernel_Sample a = _kernelData[i - 1];

			float d = b.x - a.x;
			float dx = _x - a.x;

			float t = dx / d;

			v.x = a.r * (1 - t) + b.r * t;
			v.y = a.g * (1 - t) + b.g * t;
			v.z = a.b * (1 - t) + b.b * t;
		}

		return v;
	}

	void calculate_ssss_discr_sep_kernel(const std::vector<Kernel_Sample>& _kernelData)
	{
		const float EXPONENT = 2.0f; // used for impartance sampling

		float RANGE = _kernelData[_kernelData.size() - 1].x; // get max. sample location

		// calculate offsets
		std::vector<float> offsets;
		calculate_offset(RANGE, EXPONENT, num_samples, offsets);

		// calculate areas (using importance-sampling) 
		std::vector<float> areas;
		calculate_areas(offsets, areas);

		kernel.resize(num_samples);

		dx::XMFLOAT3 sum = dx::XMFLOAT3(0, 0, 0); // weights sum for normalization

		// compute interpolated weights
		for (int i = 0; i < num_samples; i++)
		{
			float sx = offsets[i];

			dx::XMFLOAT3 v = lin_interpol_1d(_kernelData, sx);
			kernel[i].x = v.x * areas[i];
			kernel[i].y = v.y * areas[i];
			kernel[i].z = v.z * areas[i];
			kernel[i].w = sx;

			sum.x += kernel[i].x;
			sum.y += kernel[i].y;
			sum.z += kernel[i].z;
		}

		// Normalize
		for (int i = 0; i < num_samples; i++)
		{
			kernel[i].x /= sum.x;
			kernel[i].y /= sum.y;
			kernel[i].z /= sum.z;
		}

		// TEMP put center at first
		dx::XMFLOAT4 t = kernel[num_samples / 2];
		for (int i = num_samples / 2; i > 0; i--)
			kernel[i] = kernel[i - 1];
		kernel[0] = t;

		// set shader vars
		//HRESULT hr;
		//V(effect->GetVariableByName("maxOffsetMm")->AsScalar()->SetFloat(RANGE));
		//V(kernelVariable->SetFloatVectorArray((float*)&kernel.front(), 0, nSamples));
	}

	void override_ssss_discr_sep_kernel(const std::vector<float>& _kernel_data)
	{
		//bool use_img_2d_kernel = false;

		// conversion of linear kernel data to sample array
		std::vector<Kernel_Sample> k;
		unsigned int size = _kernel_data.size() / 4;

		unsigned int i = 0;
		for (unsigned int s = 0; s < size; s++)
		{
			Kernel_Sample ks;
			ks.r = _kernel_data[i++];
			ks.g = _kernel_data[i++];
			ks.b = _kernel_data[i++];
			ks.x = _kernel_data[i++];
			k.push_back(ks);
		}

		// kernel computation
		calculate_ssss_discr_sep_kernel(k);
	}

	dx::XMFLOAT3 gauss_1D(float x, dx::XMFLOAT3 variance)
	{
		dx::XMVECTOR var = dx::XMLoadFloat3(&variance);
		const float eul = 2.7182818284f;
		dx::XMVECTOR eul_vec = dx::XMVectorSet(eul, eul, eul, eul);
		dx::XMVECTOR sample_gauss_vec = (1 / (var * dx::XM_PI * 2)) * dx::XMVectorPow(eul_vec, (-(x * x)) / (2 * (var * var)));

		dx::XMFLOAT3 gs1D{};
		dx::XMStoreFloat3(&gs1D, sample_gauss_vec);

		return gs1D;
		//return rcp(sqrt(2.0f * dx::XM_PI) * variance) * exp(-(x * x) * rcp(2.0f * variance * variance));
	}

	dx::XMFLOAT3 profile(float r)
	{
		/**
		 * We used the red channel of the original skin profile defined in
		 * [d'Eon07] for all three channels. We noticed it can be used for green
		 * and blue channels (scaled using the falloff parameter) without
		 * introducing noticeable differences and allowing for total control over
		 * the profile. For example, it allows to create blue SSS gradients, which
		 * could be useful in case of rendering blue creatures.
		 */
		 //return  // 0.233f * gaussian(0.0064f, r) + /* We consider this one to be directly bounced light, accounted by the strength parameter (see @STRENGTH) */
	   /*      0.100f * gaussian(0.0484f, r) +
			 0.118f * gaussian(0.187f, r) +
			 0.113f * gaussian(0.567f, r) +
			 0.358f * gaussian(1.99f, r) +
			 0.078f * gaussian(7.41f, r);*/
		const float global_scale = 9.27f;
		dx::XMFLOAT3 near_var = dx::XMFLOAT3(0.055f * global_scale, 0.044f * global_scale, 0.038f * global_scale);
		dx::XMFLOAT3 far_var = dx::XMFLOAT3(1.0f * global_scale, 0.2f * global_scale, 0.13f * global_scale);
		const float weight = 0.5f;
		dx::XMVECTOR weight_vec = dx::XMVectorSet(weight, weight, weight, weight);
		dx::XMVECTOR rest_weight_vec = dx::XMVectorSet(1.0f - weight, 1.0f - weight, 1.0f - weight, 1.0f - weight);

		dx::XMFLOAT3 g1_float = (gauss_1D(r, near_var));
		dx::XMFLOAT3 g2_float = (gauss_1D(r, far_var));
		dx::XMVECTOR g1 = dx::XMLoadFloat3(&g1_float);
		dx::XMVECTOR g2 = dx::XMLoadFloat3(&g2_float);

		dx::XMVECTOR res = weight_vec * g1 + rest_weight_vec * g2;

		dx::XMFLOAT3 profi{};
		dx::XMStoreFloat3(&profi, res);

		return profi;//dx::XMFLOAT3(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2]);
	}

	std::vector<float4> separable_sss_calculate_kernel()
	{

		const float RANGE = 9.9f;//num_samples > 20 ? 3.0f : 2.0f;
		const float EXPONENT = 2.0f;

		kernel.resize(num_samples);

		// Calculate the offsets:
		float step = 2.0f * RANGE / (num_samples - 1);
		for (int i = 0; i < num_samples; i++)
		{
			float o = -RANGE + float(i) * step;
			float sign = o < 0.0f ? -1.0f : 1.0f;
			kernel[i].w = RANGE * sign * abs(pow(o, EXPONENT)) / pow(RANGE, EXPONENT);
		}

		// Calculate the weights:
		for (int i = 0; i < num_samples; i++)
		{
			float w0 = i > 0 ? abs(kernel[i].w - kernel[i - 1].w) : 0.0f;
			float w1 = i < num_samples - 1 ? abs(kernel[i].w - kernel[i + 1].w) : 0.0f;
			float area = (w0 + w1) / 2.0f;
			dx::XMFLOAT3 prof = profile(kernel[i].w);
			dx::XMFLOAT3 t = dx::XMFLOAT3(area * prof.x, area * prof.y, area * prof.z);
			kernel[i].x = t.x;
			kernel[i].y = t.y;
			kernel[i].z = t.z;
		}

		// We want the offset 0.0 to come first:
		dx::XMFLOAT4 t = kernel[num_samples / 2];
		for (int i = num_samples / 2; i > 0; i--)
			kernel[i] = kernel[i - 1];
		kernel[0] = t;

		// Calculate the sum of the weights, we will need to normalize them below:
		dx::XMFLOAT3 sum = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < num_samples; i++)
		{
			sum = dx::XMFLOAT3(kernel[i].x + sum.x, kernel[i].y + sum.y, kernel[i].z + sum.z);
		}

		// Normalize the weights:
		for (int i = 0; i < num_samples; i++)
		{
			kernel[i].x /= sum.x;
			kernel[i].y /= sum.y;
			kernel[i].z /= sum.z;
		}

		// Tweak them using the desired strength. The first one is:
		//     lerp(1.0, kernel[0].rgb, strength)
		//kernel[0].x = (1.0f - strength.x) * 1.0f + strength.x * kernel[0].x;
		//kernel[0].y = (1.0f - strength.y) * 1.0f + strength.y * kernel[0].y;
		//kernel[0].z = (1.0f - strength.z) * 1.0f + strength.z * kernel[0].z;

		//// The others:
		////     lerp(0.0, kernel[0].rgb, strength)
		//for (int i = 1; i < nSamples; i++) {
		//    kernel[i].x *= strength.x;
		//    kernel[i].y *= strength.y;
		//    kernel[i].z *= strength.z;
		//}

		return kernel;
	}
}

#endif // !JIMENEZ_SEPARABLE_H_
