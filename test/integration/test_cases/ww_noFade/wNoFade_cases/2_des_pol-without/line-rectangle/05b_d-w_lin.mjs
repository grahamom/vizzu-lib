import { data_14 } from '../../../../../test_data/chart_types_eu.mjs'

const testSteps = [
  (chart) =>
    chart.animate({
      data: data_14,
      config: {
        channels: {
          x: { set: 'Year' },
          y: { set: 'Value 2 (+)' },
          color: { set: 'Country' },
          size: { set: 'Value 3 (+)' }
        },
        title: 'Line Chart',
        geometry: 'line'
      }
    }),

  (chart) =>
    chart.animate({
      config: {
        channels: {
          x: { set: 'Year' },
          y: { set: 'Value 2 (+)' },
          color: null,
          size: { set: ['Country', 'Value 3 (+)'] }
        },
        title: 'Stack Disc & Remove Color'
      }
    }),

  (chart) =>
    chart.animate({
      config: {
        channels: {
          x: { set: 'Year' },
          y: { set: 'Value 2 (+)' },
          color: { set: null },
          size: { set: ['Country', 'Value 1 (+)'] }
        },
        title: 'Change Conti'
      }
    }),

  (chart) =>
    chart.animate(
      {
        config: {
          channels: {
            x: { set: 'Year' },
            y: { set: 'Value 2 (+)' },
            color: null,
            size: { set: ['Joy factors', 'Value 1 (+)'] }
          },
          title: 'Change Disc'
        }
      },
      {
        duration: 0
      }
    ),

  (chart) =>
    chart.animate({
      config: {
        channels: {
          x: { set: 'Year' },
          y: { set: 'Value 2 (+)' },
          color: { set: 'Joy factors' },
          size: { set: 'Value 1 (+)' }
        },
        title: 'Group new Disc & Add new Disc Color'
      }
    }),

  (chart) =>
    chart.animate(
      {
        config: {
          channels: {
            x: null,
            y: null,
            color: { set: 'Joy factors' },
            size: { set: ['Year', 'Value 3 (+)'] },
            lightness: { set: 'Value 1 (+)' }
          },
          title: 'Stack new Disc & Change Geoms & CoordSys',
          geometry: 'rectangle'
        }
      },
      {
        geometry: {
          delay: 0.5,
          duration: 1.5
        },
        x: {
          delay: 1
        }
      }
    )
]

export default testSteps
