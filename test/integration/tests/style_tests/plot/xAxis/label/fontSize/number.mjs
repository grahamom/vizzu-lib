import Chart from '../../../../../chart.mjs'

const testCases = [
  {
    testName: 'static_1',
    testSteps: [
      Chart.animate({
        data: 'simpleDataWithOneValue',
        config: 'simpleData',
        style: {
          plot: {
            xAxis: {
              label: {
                fontSize: 1
              }
            }
          }
        }
      })
    ]
  },
  {
    testName: 'static_10',
    testSteps: [
      Chart.animate({
        data: 'simpleDataWithOneValue',
        config: 'simpleData',
        style: {
          plot: {
            xAxis: {
              label: {
                fontSize: 10
              }
            }
          }
        }
      })
    ]
  },
  {
    testName: 'static_50',
    testSteps: [
      Chart.animate({
        data: 'simpleDataWithOneValue',
        config: 'simpleData',
        style: {
          plot: {
            xAxis: {
              label: {
                fontSize: 50
              }
            }
          }
        }
      })
    ]
  },
  {
    testName: 'animated_1-50',
    testSteps: [
      Chart.animate({
        data: 'simpleDataWithOneValue',
        config: 'simpleData',
        style: {
          plot: {
            xAxis: {
              label: {
                fontSize: 1
              }
            }
          }
        }
      }),
      Chart.animate({
        style: {
          plot: {
            xAxis: {
              label: {
                fontSize: 50
              }
            }
          }
        }
      })
    ]
  },
  {
    testName: 'animated_50-1',
    testSteps: [
      Chart.animate({
        data: 'simpleDataWithOneValue',
        config: 'simpleData',
        style: {
          plot: {
            xAxis: {
              label: {
                fontSize: 50
              }
            }
          }
        }
      }),
      Chart.animate({
        style: {
          plot: {
            xAxis: {
              label: {
                fontSize: 1
              }
            }
          }
        }
      })
    ]
  }
]

export default testCases
