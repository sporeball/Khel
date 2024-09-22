use crate::{KhelState, zero_to_two, chart::{Chart, ChartInfo, ChartStatus}};
use std::time::Duration;
use egui::{epaint::Shadow, style::{Spacing, Style}, Button, Color32, Context, Frame, Label, Margin, RichText, Rounding, Slider, TextEdit, Vec2};
use egui_wgpu::Renderer;
use log::error;
// use log::info;
// use log::warn;
use wgpu::{Device, TextureFormat};
use winit::{event::WindowEvent, window::Window};

pub struct EguiRenderer {
  pub context: Context,
  pub state: egui_winit::State,
  pub renderer: Renderer,
}

impl EguiRenderer {
  pub fn new(
    device: &Device,
    output_color_format: TextureFormat,
    output_depth_format: Option<TextureFormat>,
    msaa_samples: u32,
    window: &Window,
  ) -> Self {
    let context = Context::default();
    let id = context.viewport_id();
    let state = egui_winit::State::new(context.clone(), id, &window, None, None);
    let renderer = Renderer::new(
      device,
      output_color_format,
      output_depth_format,
      msaa_samples,
    );
    Self {
      context,
      state,
      renderer,
    }
  }
  pub fn handle_input(&mut self, window: &Window, event: &WindowEvent) {
    let _ = self.state.on_window_event(window, event);
  }
}

/// Create a CentralPanel with some kind of GUI inside.
/// This function is defined outside EguiRenderer so that the closure passed
/// to CentralPanel::show can access the methods on KhelState directly.
pub fn gui(state: &mut KhelState) {
  let ctx = &state.egui.context;
  // TODO: parameterize
  ctx.set_style(Style {
    spacing: Spacing {
      item_spacing: Vec2 { x: 2.0, y: 4.0 },
      ..Default::default()
    },
    ..Default::default()
  });
  egui::CentralPanel::default()
    .frame(Frame {
      outer_margin: Margin::same(10.0),
      rounding: Rounding::ZERO,
      shadow: Shadow::NONE,
      fill: Color32::TRANSPARENT,
      // stroke: Stroke::NONE,
      ..Default::default()
    })
    .show(ctx, |ui| {
      ui.add(Label::new(format!("{:.0} fps", state.fps.avg())));
      ui.add(Label::new(format!(
        "itick: {} htick: {} etick: {}",
        state.chart_info.instance_tick,
        state.chart_info.hit_tick,
        state.chart_info.end_tick,
      )));
      ui.add(TextEdit::singleline(&mut state.chart_path).hint_text("Chart"));
      if ui.add(Button::new("Play")).clicked() {
        let chart = Chart::read_from_disk(&state.chart_path);
        if let Err(chart_error) = chart {
          error!("{}", chart_error);
          state.error = Some(chart_error);
          return;
        };
        state.error = None;
        state.chart_info = ChartInfo::new(chart.unwrap());
        // state.timing_info = None;
        let chart_info = &mut state.chart_info;
        // if let ChartStatus::Playing = chart_info.status {
        //   warn!("chart is already playing");
        //   return;
        // }
        let chart = &chart_info.chart;
        // let ticks = &chart.ticks.0;
        // let Some(first_tick) = ticks.get(0) else {
        //   warn!("could not play chart, is it empty?");
        //   return;
        // };
        // let Some(first_tick) = ticks.get(0) else { unreachable!(); };
        // let start_bpm = chart.metadata.bpms.at_tick(0).value * state.ratemod as f64;
        // let one_minute = Duration::from_secs(60);
        // let one_beat = one_minute.div_f64(start_bpm);
        // let (_, ho_height) = zero_to_two(32, 32, state.size);
        // let heights_to_travel = 1.0 / ho_height;
        // let travel_time = one_beat.mul_f32(heights_to_travel).div_f32(state.xmod);
        let travel_time = Duration::from_secs_f32((state.size.height as f32 * 0.5) / state.av as f32);
        // set times
        let start_time = state.time;
        // the music should begin playing once the first hit object has finished traveling
        let music_time = start_time + travel_time;
        chart_info.start_time = start_time;
        chart_info.music_time = music_time;
        // set tick info
        let timing_info_list = chart.ticks.get_timing_info_list(
          chart.metadata.bpms.clone(),
          chart.metadata.divisors.clone(),
          start_time,
          music_time,
          travel_time,
          state.ratemod
        );
        // info!("start_time: {:?}", start_time);
        // info!("music_time: {:?}", music_time);
        // info!("travel_time: {:?}", travel_time);
        state.timing_info_list = Some(timing_info_list);
        // info!("{:?}", state.tick_info);
        chart.play(state.ratemod);
        chart_info.status = ChartStatus::Playing;
      }
      // ui.add_enabled(
      //   !matches!(state.chart_info.status, ChartStatus::Playing),
      //   Slider::new(&mut state.xmod, 1.0..=6.0).step_by(0.05).text("Xmod")
      // );
      ui.add_enabled(
        !matches!(state.chart_info.status, ChartStatus::Playing),
        Slider::new(&mut state.av, 300..=1200).step_by(1.0).text("AV")
      );
      ui.add_enabled(
        !matches!(state.chart_info.status, ChartStatus::Playing),
        Slider::new(&mut state.ratemod, 0.5..=2.0).step_by(0.1).text("Ratemod")
      );
      if state.error.is_some() {
        ui.add(Label::new(RichText::new(format!("error: {}", state.error.as_ref().unwrap())).color(Color32::RED)));
      }
      ui.add_visible_ui(
        matches!(state.chart_info.status, ChartStatus::Playing),
        |ui| {
          let current_bpm_string = match state.chart_info.chart.metadata.bpms.0.len() {
            0 => String::new(),
            _ => {
              // TODO: is this a bug?
              let hit_tick_u32 = state.chart_info.hit_tick.saturating_sub(1);
              let current_bpm = state.chart_info.chart.metadata.bpms.at_tick(hit_tick_u32).value * state.ratemod as f64;
              format!("{:.2}", current_bpm)
            },
          };
          ui.vertical_centered(|ui| ui.label(RichText::new(current_bpm_string).size(20.0)));
        }
      );
    });
}
