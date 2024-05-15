use anyhow::*;
use image::{DynamicImage, GenericImageView};
use wgpu::{AddressMode, Device, Extent3d, FilterMode, ImageCopyTexture, ImageDataLayout, Origin3d, Queue, Sampler, SamplerDescriptor, TextureAspect, TextureDescriptor, TextureDimension, TextureFormat, TextureUsages, TextureView, TextureViewDescriptor};

pub struct Texture {
  pub texture: wgpu::Texture,
  pub view: TextureView,
  pub sampler: Sampler,
}

impl Texture {
  pub fn from_bytes(device: &Device, queue: &Queue, bytes: &[u8], label: &str) -> Result<Self> {
    let img = image::load_from_memory(bytes)?;
    Self::from_image(device, queue, &img, Some(label))
  }
  pub fn from_image(device: &Device, queue: &Queue, img: &DynamicImage, label: Option<&str>) -> Result<Self> {
    let rgba = img.to_rgba8();
    let dimensions = img.dimensions();
    let size = Extent3d {
      width: dimensions.0,
      height: dimensions.1,
      depth_or_array_layers: 1,
    };
    let texture = device.create_texture(&TextureDescriptor {
      label,
      size,
      mip_level_count: 1,
      sample_count: 1,
      dimension: TextureDimension::D2,
      format: TextureFormat::Rgba8UnormSrgb,
      usage: TextureUsages::TEXTURE_BINDING | TextureUsages::COPY_DST,
      view_formats: &[],
    });
    queue.write_texture(
      ImageCopyTexture {
        aspect: TextureAspect::All,
        texture: &texture,
        mip_level: 0,
        origin: Origin3d::ZERO,
      },
      &rgba,
      ImageDataLayout {
        offset: 0,
        bytes_per_row: Some(4 * dimensions.0),
        rows_per_image: Some(dimensions.1),
      },
      size,
    );
    let view = texture.create_view(&TextureViewDescriptor::default());
    let sampler = device.create_sampler(&SamplerDescriptor {
      address_mode_u: AddressMode::ClampToEdge,
      address_mode_v: AddressMode::ClampToEdge,
      address_mode_w: AddressMode::ClampToEdge,
      mag_filter: FilterMode::Linear,
      min_filter: FilterMode::Nearest,
      mipmap_filter: FilterMode::Nearest,
      ..Default::default()
    });
    Ok(Self { texture, view, sampler })
  }
}