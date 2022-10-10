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


void Renderer::Render(const Scene& scene, const Camera& camera)
{	
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++) {
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++) {

			glm::vec4 color = PerPixel(x, y);

			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	int bounces = 2;
	float multiplier = 1.0f;

	glm::vec3 color(0.0f);

	for (int i = 0; i < bounces; i++) {

		HitPayload payload = TraceRay(ray);

		if (payload.HitDistance < 0) {
			glm::vec3 skyColor = glm::vec3(0.0f, 0.0f, 0.0f);
			color += skyColor * multiplier;
			break;
		}

		glm::vec3 lightDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));

		float d = glm::max(glm::dot(payload.WorldNormal, -lightDirection), 0.0f); // = cos(angle)

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];

		glm::vec3 sphereColor = sphere.Albedo;
		sphereColor *= d;
		color += sphereColor * multiplier;
		multiplier *= 0.7f;

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal);
	}

	return glm::vec4(color, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray) {

	//(b.x^2 + b.y^2 + b.z^2)t^2 + 2(a.xb.x + a.yb.y + a.zb.z)t + (a.x^2 + a.y^2 + a.z^2 - r^2)
	//a - ray origin vector
	//b - ray direction vector
	//r - radius
	//t - distance

	//float a = ray.Direction.x * ray.Direction.x + ray.Direction.y * ray.irection.y + ray.Direction.z * ray.Direction.z; ==> dot product
	//float b = 2*(ray.Direction.x * ray.Origin.x + ray.Direction.y * ray.Origin.y + ray.Direction.z * ray.Origin.z); ==> also dot product
	//float c = ray.Origin.x * ray.Origin.x + ray.Origin.y * ray.Origin.y + ray.Origin.z * ray.Origin.z - radius* radius; ==> dot product

	size_t closestSphereIndex = -1;
	float hitDistance = FLT_MAX;

	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++) {

		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		//delta = b^2 - 4ac
		float delta = b * b - 4.0f * a * c;

		if (delta < 0.0f)
			continue;

		float closestT = (-b - glm::sqrt(delta)) / (2.0f * a);
		//float t0 = (-b + glm::sqrt(delta)) / (2.0f * a);

		if (closestT > 0.0f && closestT < hitDistance) {
			hitDistance = closestT;
			closestSphereIndex = i;
		}
	}

	if (closestSphereIndex == -1)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphereIndex);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, size_t objectIndex)
{
	HitPayload payload;
	payload.ObjectIndex = objectIndex;
	payload.HitDistance = hitDistance;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;

	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	HitPayload payload;
	payload.HitDistance = -1;

	return payload;
}