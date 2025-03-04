import { data_14 } from '../../../../../test_data/chart_types_eu.mjs'

const testSteps = [
  (chart) =>
    chart.animate({
      data: data_14,
      config: {
        channels: {
          x: { set: 'Year' },
          y: { set: 'Value 2 (+)' },
          color: { set: 'Country' }
        },
        title: 'Line Chart',
        geometry: 'line'
      }
    }),

  (chart) =>
    chart.animate({
      config: {
        channels: {
          color: null,
          size: { set: 'Country' }
        },
        title: 'Stack Disc & Remove Color'
      }
    }),

  (chart) =>
    chart.animate(
      {
        config: {
          channels: {
            size: { set: 'Joy factors' }
          },
          title: 'Change Discrete'
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
          y: { set: 'Value 1 (+)' },
          color: null,
          size: { set: 'Joy factors' }
        },
        title: 'Change Conti'
      }
    }),

  (chart) =>
    chart.animate({
      config: {
        channels: {
          x: { set: 'Year' },
          y: { set: 'Value 1 (+)' },
          color: { set: 'Joy factors' },
          size: null
        },
        title: 'Group new Disc & Add new Disc Color'
      }
    })
]

export default testSteps
