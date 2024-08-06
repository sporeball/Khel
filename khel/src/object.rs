use crate::{load_binary, texture::{self, DrawTexture, Texture}, Instance};
use std::collections::HashMap;
use std::time::Duration;
use wgpu::{util::{BufferInitDescriptor, DeviceExt}, Buffer, BufferUsages, Device, Queue, RenderPass};
use winit::dpi::PhysicalSize;

pub struct Object {
  pub texture: Texture,
  pub instances: HashMap<u32, Instance>,
  pub instance_buffer: Buffer,
}

impl Object {
  pub fn from_file(filename: &str, window_size: PhysicalSize<u32>, device: &Device, queue: &Queue) -> Result<Self, anyhow::Error> {
    let bytes = load_binary(filename).unwrap();
    let texture = texture::Texture::from_bytes(window_size, &device, &queue, &bytes, filename).unwrap();
    // let instances = vec![];
    let instances: HashMap<u32, Instance> = HashMap::new();
    let instance_buffer = create_instance_buffer(&instances, device);
    Ok(Self {
      texture,
      instances,
      instance_buffer,
    })
  }
}

#[derive(Default)]
pub struct Objects {
  pub map: HashMap<String, Object>,
  // TODO
  pub to_be_destroyed: HashMap<u32, Duration>,
}

// TODO: is it possible to put this on Object? i forgot...
// pub fn create_instance_buffer(instances: &Vec<Instance>, device: &Device) -> Buffer {
pub fn create_instance_buffer(instances: &HashMap<u32, Instance>, device: &Device) -> Buffer {
  // let instance_data = instances
  //   .iter()
  //   .map(Instance::to_raw)
  //   .collect::<Vec<_>>();
  let instance_data = instances
    .values()
    .cloned()
    .collect::<Vec<_>>()
    .iter()
    .map(Instance::to_raw)
    .collect::<Vec<_>>();
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
