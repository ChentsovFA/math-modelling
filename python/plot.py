import sys
import os
from heat_equation_plotter import HeatEquationPlotter

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'plotters'))

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Usage: plot.py <plotter> <json_data_path> <output_path>')
        raise SystemError

    plotters = {
        'heat_equation': HeatEquationPlotter,
    }

    if sys.argv[1] not in plotters:
        raise SystemError

    Plotter = plotters[sys.argv[1]]
    plotter = Plotter(sys.argv[2], sys.argv[3])
    plotter.plot()
