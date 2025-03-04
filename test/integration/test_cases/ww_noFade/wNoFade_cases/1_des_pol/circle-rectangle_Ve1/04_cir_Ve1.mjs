import { data } from '../../../../../test_data/chart_types_eu.mjs'

const testSteps = [
  (chart) =>
    chart.animate({
      data: Object.assign(data, {
        filter: (record) =>
          record.Country === 'Austria' ||
          record.Country === 'Belgium' ||
          record.Country === 'Bulgaria' ||
          record.Country === 'Cyprus' ||
          record.Country === 'Czechia' ||
          record.Country === 'Denmark' ||
          record.Country === 'Estonia' ||
          record.Country === 'Greece' ||
          record.Country === 'Germany' ||
          record.Country === 'Spain' ||
          record.Country === 'Finland' ||
          record.Country === 'France' ||
          record.Country === 'Croatia' ||
          record.Country === 'Hungary'
      }),
      config: {
        channels: {
          y: { set: 'Value 3 (+)' },
          x: { set: 'Value 2 (+)' },
          color: { set: 'Country' },
          noop: { set: 'Year' },
          size: { set: 'Value 1 (+)' }
        },
        title: 'Dot plot',
        geometry: 'circle'
      }
    }),

  (chart) =>
    chart.animate({
      config: {
        channels: {
          noop: null,
          size: { set: ['Value 1 (+)', 'Year'] }
        },
        title: 'Change Discrete'
      }
    }),

  (chart) =>
    chart.animate(
      {
        config: {
          channels: {
            noop: null,
            size: { set: ['Value 1 (+)', 'Joy factors'] }
          },
          title: 'Change Discrete'
        }
      },
      {
        duration: 0.5
      }
    ),

  (chart) =>
    chart.animate(
      {
        config: {
          channels: {
            x: { set: ['Country', 'Value 2 (+)'] },
            y: { set: ['Joy factors', 'Value 3 (+)'] },
            noop: { set: 'Joy factors' },
            size: { set: 'Value 1 (+)' }
          },
          title: 'Change Geometry',
          geometry: 'rectangle',
          orientation: 'vertical'
        }
      },
      {
        geometry: {
          delay: 0.7,
          duration: 1
        }
      }
    )
]

export default testSteps
