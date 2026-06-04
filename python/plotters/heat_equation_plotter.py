import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from abstract_plotter import AbstractPlotter
import json


class HeatEquationPlotter(AbstractPlotter):
    def __init__(self, json_data_path, output_path):
        super().__init__(json_data_path, output_path)

        # Загрузка данных
        if isinstance(self.data, str):
            with open(self.data) as f:
                self.data = json.load(f)

        self.frames = []
        self.x = None
        self.y = None

        # Парсинг данных (поддерживает оба формата)
        if "data" in self.data:
            # Данные с временными слоями (через Solve())
            for frame_data in self.data["data"]:
                field = np.array(frame_data["data"]["grid"])
                field_clean = np.where(field is None, np.nan, field).astype(float)
                self.frames.append({
                    'time': frame_data["time"],
                    'field': field_clean
                })
                if self.x is None:
                    N = field.shape[1]
                    # область 3×3
                    self.x = np.linspace(0, 3, N)
                    self.y = np.linspace(0, 3, N)
        else:
            # Данные с одним слоем (через ExportData напрямую)
            field = np.array(self.data["grid"])
            field_clean = np.where(field is None, np.nan, field).astype(float)
            self.frames.append({
                'time': self.data.get("current_time", 0),
                'field': field_clean
            })
            N = field.shape[1]
            # область 3×3
            self.x = np.linspace(0, 3, N)
            self.y = np.linspace(0, 3, N)

    def plot(self):
        """Создание анимации распределения температуры."""

        X, Y = np.meshgrid(self.x, self.y)

        # Настройка фигуры
        fig, ax = plt.subplots(figsize=(10, 8))

        # Определение диапазона цветов
        all_values = []
        for frame in self.frames:
            valid_values = frame['field'][~np.isnan(frame['field'])]
            if len(valid_values) > 0:
                all_values.extend(valid_values)

        vmin = min(all_values) if all_values else -1
        vmax = max(all_values) if all_values else 4

        # Создание контурного графика
        contour = ax.contourf(
            X, Y, self.frames[0]['field'],
            levels=50, cmap='hot', vmin=vmin, vmax=vmax
        )
        cbar = fig.colorbar(contour, ax=ax)
        cbar.set_label('Temperature')

        # отображение вырезанной области [2,3]×[1,2]
        cutout_x = [2, 3, 3, 2, 2]
        cutout_y = [1, 1, 2, 2, 1]
        ax.fill(cutout_x, cutout_y, 'gray', alpha=0.5, label='Cutout region')

        # границы области
        ax.plot([0, 3], [0, 0], 'k-', linewidth=2)   # нижняя
        ax.plot([0, 0], [0, 3], 'k-', linewidth=2)   # левая
        ax.plot([0, 3], [3, 3], 'k-', linewidth=2)   # верхняя
        ax.plot([3, 3], [0, 3], 'k-', linewidth=2)   # правая

        # границы выреза (пунктиром)
        ax.plot([2, 3], [2, 2], 'k--', linewidth=1, alpha=0.5)
        ax.plot([2, 2], [1, 2], 'k--', linewidth=1, alpha=0.5)
        ax.plot([2, 3], [1, 1], 'k--', linewidth=1, alpha=0.5)

        ax.set_xlabel('x')
        ax.set_ylabel('y')
        ax.set_xlim(0, 3)  
        ax.set_ylim(0, 3)  
        ax.set_aspect('equal')
        ax.grid(True, alpha=0.3)

        time_text = ax.text(
            0.02, 0.98, '', transform=ax.transAxes,
            fontsize=12, verticalalignment='top'
        )

        def update(frame_idx):
            """Обновление кадра анимации."""
            frame = self.frames[frame_idx]

            # Очистка и перерисовка контуров
            for coll in contour.collections:
                coll.remove()

            new_contour = ax.contourf(
                X, Y, frame['field'],
                levels=50, cmap='hot', vmin=vmin, vmax=vmax
            )
            contour.collections = new_contour.collections

            time_text.set_text(f't = {frame["time"]:.3f}')
            ax.set_title(f'Heat Distribution at t = {frame["time"]:.3f}')

            return [time_text] + list(contour.collections)

        # Создание анимации
        if len(self.frames) > 1:
            ani = animation.FuncAnimation(
                fig, update, frames=len(self.frames),
                interval=100, blit=True, repeat=True
            )

            # Сохранение видео
            if self.output_path.endswith('.mp4'):
                writer = animation.FFMpegWriter(fps=10, bitrate=1800)
                ani.save(self.output_path, writer=writer)
            else:
                ani.save(self.output_path, writer='pillow', fps=10)
        else:
            # Сохранение одного кадра
            plt.savefig(self.output_path, dpi=150, bbox_inches='tight')

        plt.close()
        print(f"Animation saved to {self.output_path}")
