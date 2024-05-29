use crate::{load_binary, texture::{self, Texture, DrawTexture}, Instance};
use cgmath::Vector3;
use wgpu::{util::{BufferInitDescriptor, DeviceExt}, Buffer, BufferUsages, Device, Queue, RenderPass};
use winit::dpi::PhysicalSize;

pub struct Object {
  pub texture: Texture,
  pub instances: Vec<Instance>,
  pub instance_buffer: Buffer,
}

impl Object {
  pub fn from_file(filename: &str, window_size: PhysicalSize<u32>, device: &Device, queue: &Queue) -> Result<Self, anyhow::Error> {
    let bytes = load_binary(filename).unwrap();
    let texture = texture::Texture::from_bytes(window_size, &device, &queue, &bytes, filename).unwrap();
    let instances = vec![];
    let instance_buffer = create_instance_buffer(&instances, device);
    Ok(Self {
      texture,
      instances,
      instance_buffer,
    })
  }
  pub fn instantiate(&mut self, x: f32, y: f32, device: &Device) -> usize {
    let instance = Instance {
      position: Vector3 { x, y, z: 0.0 },
    };
    self.instances.push(instance);
    self.instance_buffer = create_instance_buffer(&self.instances, &device);
    // Ok(self.instances.len())
    self.instances.len()
  }
}

pub fn create_instance_buffer(instances: &Vec<Instance>, device: &Device) -> Buffer {
  let instance_data = instances.iter().map(Instance::to_raw).collect::<Vec<_>>();
  let instance_buffer = device.create_buffer_init(&BufferInitDescriptor {
    label: Some("Instance Buffer"),
    contents: bytemuck::cast_slice(&instance_data),
    usage: BufferUsages::VERTEX,
  });
  instance_buffer
}

pub trait DrawObject<'a> {
  fn draw_object_instanced(&mut self, object: &'a Object);
}

impl<'a, 'b> DrawObject<'b> for RenderPass<'a>
where
  'b: 'a,
{
  fn draw_object_instanced(&mut self, object: &'b Object) {
    self.set_vertex_buffer(1, object.instance_buffer.slice(..));
    self.draw_texture_instanced(&object.texture, 0..object.instances.len() as u32);
  }
}