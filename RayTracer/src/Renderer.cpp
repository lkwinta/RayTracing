#include "Renderer.h"

namespace Utils {
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;

		return result;
	}

}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage) {
		//no resize neccesery
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}


void Renderer::Render()
{	
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++) {
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++) {
			glm::vec2 coord = { (float)x / (float) m_FinalImage->GetWidth(), (float)y / (float) m_FinalImage->GetHeight() };
			coord = coord * 2.0f - 1.0f; //remap coord to -1, 1
			//coord.x *= m_AspectRatio;

			glm::vec4 color = PerPixel(coord);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::PerPixel(glm::vec2 coord) {

	glm::vec3 rayOrigin(0.0f, 0.0f, 1.0f);
	glm::vec3 rayDirection(coord.x, coord.y, -1.0f);
	

	float radius = 0.5f;

	//rayDirection = glm::normalize(rayDirection);

	//(b.x^2 + b.y^2 + b.z^2)t^2 + 2(a.xb.x + a.yb.y + a.zb.z)t + (a.x^2 + a.y^2 + a.z^2 - r^2)
	//a - ray origin vector
	//b - ray direction vector
	//r - radius
	//t - distance

	//float a = rayDirection.x * rayDirection.x + rayDirection.y * rayDirection.y + rayDirection.z * rayDirection.z; ==> dot product
	//float b = 2*(rayDirection.x * rayOrigin.x + rayDirection.y * rayOrigin.y + rayDirection.z * rayOrigin.z); ==> also dot product
	//float c = rayOrigin.x * rayOrigin.x + rayOrigin.y * rayOrigin.y + rayOrigin.z * rayOrigin.z - radius* radius; ==> dot product

	float a = glm::dot(rayDirection, rayDirection);
	float b = 2.0f*glm::dot(rayOrigin, rayDirection);
	float c = glm::dot(rayOrigin, rayOrigin) - radius * radius;

	//delta = b^2 - 4ac
	float delta = b * b - 4.0f * a * c;

	if (delta < 0.0f)
		return glm::vec4(0, 0, 0, 1);

	float closestT = (-b - glm::sqrt(delta)) / (2.0f * a);
	float t0 = (-b + glm::sqrt(delta)) / (2.0f * a);
	

	glm::vec3 hitPoint = rayOrigin + rayDirection * closestT;
	glm::vec3 normal = glm::normalize(hitPoint);

	glm::vec3 lightDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));

	float d = glm::max(glm::dot(normal, -lightDirection), 0.0f); // = cos(angle)
 
	glm::vec3 sphereColor(1.0f, 0, 1.0f);
	sphereColor *= d;
	//sphereColor = normal * 0.5f + 0.5f;

	return glm::vec4(sphereColor, 1.0f);
}
