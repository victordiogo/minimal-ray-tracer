#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>

#include <glm/glm.hpp>

struct Sphere {
  float radius;
  glm::vec3 center;
};

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;
};

auto save_image(const std::vector<glm::vec3>& image, unsigned width, unsigned height) -> void {
  auto file = std::ofstream{"output.ppm", std::ios::binary};
  file << "P6\n" << width << ' ' << height << "\n255\n";

  for (auto i = 0U; i < width * height; ++i) {
    auto color = image[i];
    auto r = static_cast<int>(255.99 * std::clamp(color.r, 0.0F, 1.0F));
    auto g = static_cast<int>(255.99 * std::clamp(color.g, 0.0F, 1.0F));
    auto b = static_cast<int>(255.99 * std::clamp(color.b, 0.0F, 1.0F));

    file << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);
  }
}

struct Intersection {
  unsigned sphere_index;
  float distance;
};

auto trace(Ray ray, const std::vector<Sphere>& spheres) -> std::optional<Intersection> {
  auto nearest = Intersection{-1U, std::numeric_limits<float>::max()};

  for (auto i = 0U; i < spheres.size(); ++i) {
    auto sphere = spheres[i];
    auto oc = sphere.center - ray.origin;
    auto dot = glm::dot(ray.direction, oc);
    if (dot < 0) {
      continue;
    }
    auto d2 = glm::dot(oc, oc) - dot * dot;
    auto r2 = sphere.radius * sphere.radius;
    if (d2 > r2) {
      continue;
    }
    auto distance = dot - std::sqrt(r2 - d2);

    if (distance < nearest.distance) {
      nearest = Intersection{i, distance};
    }
  }

  if (nearest.sphere_index != -1U) {
    return nearest;
  } 
  else {
    return {};
  }
}

auto ray_cast(Ray ray, const std::vector<Sphere>& spheres) -> glm::vec3 {
  auto intersection = trace(ray, spheres);

  if (intersection) {
    auto sphere = spheres[intersection->sphere_index];
    auto distance = intersection->distance;

    auto point = ray.origin + ray.direction * distance;
    auto light_dir = glm::normalize(glm::vec3{1.0, -1.0, -1.0});
    auto normal = glm::normalize(point - sphere.center);
    auto texture = glm::vec2{};
    texture.x = (1.0F + std::atan2(normal.z, normal.x) / 3.1415F) * 0.5F;
    texture.y = std::acos(normal.y) / 3.1415F;
    auto pattern = (static_cast<int>(texture.x * 10) + static_cast<int>(texture.y * 10)) % 2 == 0;
    auto ambient = glm::vec3{0.1, 0.1, 0.1};

    auto shadow_ray = Ray{point, -light_dir};
    auto shadow_intersection = trace(shadow_ray, spheres);
    if (shadow_intersection) {
      return ambient * (pattern ? 0.5F : 1.0F);
    }

    auto diffuse = std::max(0.0F, glm::dot(normal, -light_dir)) * 0.8F;
    auto reflection = glm::reflect(ray.direction, normal);
    auto specular = std::powf(std::max(0.0F, glm::dot(reflection, -light_dir)), 32) * 0.5F;

    return (ambient + diffuse) * (pattern ? 0.5F : 1.0F) + specular;
  }

  return {};
}

auto render(const std::vector<Sphere>& spheres) -> void {
  constexpr auto width = 1280U;
  constexpr auto height = 720U;
  constexpr auto aspect_ratio = static_cast<float>(width) / height;
  constexpr auto fov = 3.1415F / 3;

  auto image = std::vector<glm::vec3>{width * height};

  auto start = std::chrono::high_resolution_clock::now();
  for (auto y = 0U; y < height; ++y) {
    for (auto x = 0U; x < width; ++x) {
      auto ray = Ray{};
      ray.direction.x = (2 * (static_cast<float>(x) + 0.5F) / static_cast<float>(width) - 1) * aspect_ratio;
      ray.direction.y = 1 - 2 * (static_cast<float>(y) + 0.5F) / static_cast<float>(height);
      ray.direction.z = -1 / std::tan(fov / 2);
      ray.direction = glm::normalize(ray.direction);

      image[y * width + x] = ray_cast(ray, spheres);
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << "Render time: " << duration << "ms" << std::endl;

  save_image(image, width, height);
}

auto main() -> int {
  auto spheres = std::vector<Sphere>{
    Sphere{1.0, {0.0, 1.0, -4.0}},
    Sphere{2.0, {2.0, -1.0, -8.5}},
  };

  render(spheres);

  return 0;
}