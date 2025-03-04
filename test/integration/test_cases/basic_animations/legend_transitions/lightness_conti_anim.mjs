import { data } from '../../../test_data/chart_types_eu.mjs'

const testSteps = [
  (chart) =>
    chart.animate({
      data,
      config: {
        channels: {
          y: { attach: ['Value 5 (+/-)'], range: { min: '0%', max: '110%' } },
          x: { attach: ['Value 6 (+/-)'], range: { min: '0%', max: '110%' } },
          color: { attach: ['Country'] },
          lightness: { attach: ['Value 5 (+/-)'] },
          size: { attach: ['Value 4 (+/-)'] },
          label: { attach: ['Value 5 (+/-)'] }
        },
        title: 'Lightness Continuous Anim.',
        legend: 'lightness',
        geometry: 'circle'
      }
    }),
  (chart) =>
    chart.animate({
      data: {
        filter: (record) =>
          record.Country !== 'Germany' &&
          record.Country !== 'Italy' &&
          record.Country !== 'Hungary' &&
          record.Country !== 'Netherlands'
      },

      config: {
        title: 'Lightness Continuous Anim. - Filtered'
      }
    })
]

export default testSteps
