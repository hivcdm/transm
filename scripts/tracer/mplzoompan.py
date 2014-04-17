class ZoomPan:
    def __init__(self):
        self.press = None
        self.cur_xlim = None
        self.cur_ylim = None
        self.x0 = None
        self.y0 = None
        self.x1 = None
        self.y1 = None
        self.xpress = None
        self.ypress = None


    def zoom_factory(self, ax, ax_twin, base_scale = 2.):
        def zoom(event):
            xdata = event.xdata # get event x location

            if event.button == 'down':
                scale_factor = base_scale
            elif event.button == 'up':
                scale_factor = 1 / base_scale
                
            cur_xlim = ax.get_xlim()
            cur_ylim = ax.get_ylim()

            new_width = (cur_xlim[1] - cur_xlim[0]) * scale_factor 
            relx = (cur_xlim[1] - xdata)/(cur_xlim[1] - cur_xlim[0])
            ax.set_xlim([xdata - new_width * (1-relx), xdata + new_width * (relx)])

            cur_xlim_twin = ax_twin.get_xlim()
            cur_ylim_twin = ax_twin.get_ylim()

            new_width = (cur_xlim_twin[1] - cur_xlim_twin[0]) * scale_factor 
            relx = (cur_xlim_twin[1] - xdata)/(cur_xlim_twin[1] - cur_xlim_twin[0])
            ax_twin.set_xlim([xdata - new_width * (1-relx), xdata + new_width * (relx)])

            ax.figure.canvas.draw()

        fig = ax.get_figure() # get the figure of interest
        fig.canvas.mpl_connect('scroll_event', zoom)

        return zoom

    def pan_factory(self, ax, ax_twin):
        def onPress(event):
            if event.inaxes != ax: return
            self.cur_xlim = ax.get_xlim()
            self.cur_xlim_twin = ax_twin.get_xlim()
            self.press = self.x0, self.y0, event.xdata, event.ydata
            self.x0, self.y0, self.xpress, self.ypress = self.press

        def onRelease(event):
            self.press = None
            ax.figure.canvas.draw()

        def onMotion(event):
            if self.press is None: return
            if event.inaxes != ax: return
            dx = event.xdata - self.xpress
            self.cur_xlim -= dx
            self.cur_xlim_twin -= dx
            ax.set_xlim(self.cur_xlim)
            ax_twin.set_xlim(self.cur_xlim_twin)

            ax.figure.canvas.draw()

        fig = ax.get_figure() # get the figure of interest

        # attach the call back
        fig.canvas.mpl_connect('button_press_event',onPress)
        fig.canvas.mpl_connect('button_release_event',onRelease)
        fig.canvas.mpl_connect('motion_notify_event',onMotion)

        #return the function
        return onMotion
