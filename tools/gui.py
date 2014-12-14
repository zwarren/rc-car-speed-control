#!/usr/bin/env python

import sys
import os
from PyQt4 import Qt, QtCore, QtGui
import PyQt4.Qwt5 as Qwt
import PyQt4.Qwt5.anynumpy as np
import logging
from logging import error, warning, info
import subprocess

class Connection:
    def __init__(self, update_fn, conn_type, host, port=0):

        if conn_type == 'TCP':
            self.sock = Qt.QTcpSocket() 
        elif conn_type == 'LOCAL':
            self.sock = Qt.QLocalSocket()
        
        self.conn_type = conn_type
        self.host = host
        self.port = port

        self.update_fn = update_fn
        self.retry_delay_ms = 2000

        self.sock.connected.connect(self.connected)
        self.sock.disconnected.connect(self.disconnected)
        self.sock.error.connect(self.socket_error)
        self.sock.readyRead.connect(self.data_ready)

        self.connect()

    def connect(self):
        info("Attempting to connect:")
        if self.conn_type == 'TCP':
            self.sock.connectToHost(self.host, self.port)
        elif self.conn_type == 'LOCAL':
            # need abs path, don't know why relative path doesn't work.
            self.sock.connectToServer(os.path.join(os.getcwd(), self.host))

    def connected(self):
        info('Connected to host!')

    def disconnected(self):
        info("Disconnected from host!")
        Qt.QTimer.singleShot(self.retry_delay_ms, self.connect)

    def socket_error(self, error):
        warning("Socket error:" + self.sock.errorString())
        Qt.QTimer.singleShot(self.retry_delay_ms, self.connect)

    def data_ready(self):
        while self.sock.canReadLine():
            line = str(self.sock.readLine()).strip()
            if len(line) > 0:
                self.update_fn(line)

    def send(self, data):
        self.sock.writeData(data)

class ArrowWidget(Qt.QWidget):

    press = QtCore.pyqtSignal(int)
    release = QtCore.pyqtSignal(int)

    def __init__(self, parent=None):
        Qt.QWidget.__init__(self, parent)

        self.setMinimumSize(100,100)
        self.setFocusPolicy(QtCore.Qt.ClickFocus)
        #self.setAutoFillBackground(True)
        #p = self.palette()
        #p.setColor(self.backgroundRole(), Qt.QColor.red)
        #self.setPalette(p)
       
        self.pressed = [False,False,False,False]
        self.focused = False

        arrow_points = (( 0,  10), (10, 0), (  5, 0), (5, -10),
                        (-5, -10), (-5, 0), (-10, 0), (0, 10))
 
        self.arrow_path = Qt.QPainterPath()
        self.arrow_path.moveTo(*arrow_points[0])
        for p in arrow_points[1:]:
            self.arrow_path.lineTo(*p)

        self.arrow_positions = ((0.25, 0.5,   90), (0.75, 0.5, -90),
                                (0.5,  0.25, 180), (0.5,  0.75, 0))

    def keyPressEvent(self, e):
        if e.key() == Qt.Qt.Key_Left:
            self.pressed[0] = True
            d = 0
        elif e.key() == Qt.Qt.Key_Right:
            self.pressed[1] = True
            d = 1
        elif e.key() == Qt.Qt.Key_Up:
            self.pressed[2] = True
            d = 2
        elif e.key() == Qt.Qt.Key_Down:
            self.pressed[3] = True
            d = 3
        else:
            Qt.QWidget.keyPressEvent(self, e)
            return
        self.press.emit(d)
        self.update()

    def keyReleaseEvent(self, e):
        if e.key() == Qt.Qt.Key_Left:
            self.pressed[0] = False
            d = 0
        elif e.key() == Qt.Qt.Key_Right:
            self.pressed[1] = False
            d = 1
        elif e.key() == Qt.Qt.Key_Up:
            self.pressed[2] = False
            d = 2
        elif e.key() == Qt.Qt.Key_Down:
            self.pressed[3] = False
            d = 3
        else:
            Qt.QWidget.keyPressEvent(self, e)
            return
        self.release.emit(d)
        self.update()

    def focusInEvent(self, e):
        self.focused = True
        self.update()

    def focusOutEvent(self, e):
        self.focused = False
        self.update()

    def paintEvent(self, e):
        qp = QtGui.QPainter()
        qp.begin(self)
        self.drawWidget(qp)
        qp.end()
      
    def drawWidget(self, qp):
        size = self.size()
        w = size.width()
        h = size.height()

        arrow_brush = QtGui.QBrush(QtGui.QColor(0, 255, 0))
        dash_pen = QtGui.QPen(QtGui.QColor(0,0,0))
        dash_pen.setStyle(QtCore.Qt.DashLine)
        qp.setPen(QtGui.QColor(0, 0, 0))

        if self.focused:
            qp.save()
            qp.setPen(dash_pen)
            qp.drawRect(0,0,w-1,h-1) 
            qp.restore()
       
        for n,pos in enumerate(self.arrow_positions): 
            x,y,rot = pos
            qp.save()
            qp.translate(w*x, h*y)
            qp.rotate(rot)
            
            if self.pressed[n]:
                qp.setBrush(arrow_brush)
            else:
                qp.setBrush(QtCore.Qt.NoBrush)
            
            qp.drawPath(self.arrow_path)
            qp.restore()

#convenience function, making a lot of entries
#def line_entry(name, text, entries=None, onEnterFn=None):
#    entry = Qt.QLineEdit(text)
#    entry.name = name
#    if not entries is None:
#        entries.append(entry)
#    if onEnterFn:
#        entry.returnPressed.connect(onEnterFn)
#    return entry

def add_entry_to_box(box, name, text, return_pressed_fn=None, entry_width=None, label_width=None,
                entry_right_adjust=False, label_right_adjust=False, read_only=False):

    entry = Qt.QLineEdit(text)
    if return_pressed_fn is not None:
        entry.returnPressed.connect(return_pressed_fn)    

    if entry_width is not None:
        entry.setFixedWidth(entry_width)
    if entry_right_adjust:
        entry.setAlignment(QtCore.Qt.AlignRight)

    if read_only:
        entry.setReadOnly(True)

    label = Qt.QLabel(name)
    if label_width is not None:
        label.setMinimumWidth(label_width)
    if label_right_adjust:
        label.setAlignment(QtCore.Qt.AlignRight)

    box.addWidget(label)
    box.addWidget(entry)

    return entry

def add_button_to_box(box, label, click_fn):
    button = Qt.QPushButton(label)
    button.clicked.connect(click_fn)
    box.addWidget(button)

class MainWindow(Qt.QWidget):
    def __init__(self, sock_name):
        Qt.QWidget.__init__(self)
        
        self.connection = Connection(self.update, 'LOCAL', sock_name)
       
        # strangely the line edits come up with gray background, do this to make them white. 
        self.setStyleSheet('QLineEdit { background-color: #ffffff }')

        vbox = Qt.QVBoxLayout()

        # start/stop and send button row	
        hbox = Qt.QHBoxLayout()
        self.text_entry = Qt.QLineEdit()
        self.text_entry.returnPressed.connect(self.send_line)

        add_button_to_box(hbox, 'Start', self.start_clicked)
        add_button_to_box(hbox, 'Stop', self.stop_clicked)
        hbox.addWidget(self.text_entry)
        add_button_to_box(hbox, 'Send', self.send_line)

        vbox.addLayout(hbox)

        # steering params
        hbox = Qt.QHBoxLayout()
        self.steering_entry = add_entry_to_box(hbox, 'Steering', '0', self.set_steer)
        self.steering_offset_entry = add_entry_to_box(hbox, 'Steering Offset', '0', self.set_steer_offset)
        hbox.addStretch(1)
        add_button_to_box(hbox, 'Get', self.get_steer)
        vbox.addLayout(hbox)

        # throttle params
        hbox = Qt.QHBoxLayout()
        self.throttle_entry = add_entry_to_box(hbox, 'Throttle', '0', self.set_throttle)
        self.throttle_offset_entry = add_entry_to_box(hbox, 'Throttle Offset', '0', self.set_throttle_offset)
        hbox.addStretch(1)
        add_button_to_box(hbox, 'Get', self.get_throttle)
        vbox.addLayout(hbox)

        # speed control params
        hbox = Qt.QHBoxLayout()
        self.speed_entry = add_entry_to_box(hbox, 'Target Speed', '0', self.set_speed)
        self.current_speed_entry = add_entry_to_box(hbox, 'Current Speed', '0', read_only=True)
        hbox.addStretch(1)
        vbox.addLayout(hbox)
 
        # row with live state values e.g. error, integral etc.
        hbox = Qt.QHBoxLayout()
        self.error_entry = add_entry_to_box(hbox, 'Error', '0', read_only=True)
        self.integral_entry = add_entry_to_box(hbox, 'Integral', '0', read_only=True)
        self.derivative_entry = add_entry_to_box(hbox, 'Derivative', '0', read_only=True)
        hbox.addStretch(1)
        vbox.addLayout(hbox)

        # row with PID constants 
        hbox = Qt.QHBoxLayout()
        self.Kp_entry = add_entry_to_box(hbox, 'Kp', '0', self.set_pid_params)
        self.Ki_entry = add_entry_to_box(hbox, 'Ki', '0', self.set_pid_params)
        self.Kd_entry = add_entry_to_box(hbox, 'Kd', '0', self.set_pid_params)
        self.max_integral_entry = add_entry_to_box(hbox, 'Max Integral', '0', self.set_pid_params)
        self.max_step_entry = add_entry_to_box(hbox, 'Max Step', '0', self.set_pid_params)
        hbox.addStretch(1)
        add_button_to_box(hbox, 'Get', self.get_pid_params)  
        vbox.addLayout(hbox)
        # add the arrow widget
        hbox = Qt.QHBoxLayout()
        hbox.addStretch(1)
        arrow_widget = ArrowWidget()
        hbox.addWidget(arrow_widget)
        vbox.addLayout(hbox) 

        # large text area
        self.text_area = Qt.QTextEdit()
        vbox.addWidget(self.text_area)

        self.setLayout(vbox)

    def send(self, line):
        self.text_area.append('Send:' + line)
        self.connection.send(line + '\n')

    def send_line(self):
        line = self.text_entry.text()
        if len(line) > 0:
            self.send(line)

    def start_clicked(self):
        self.send('Start')

    def stop_clicked(self):
        self.send('Stop')

    def get_steer(self):
        self.send('Get Steer')
        self.send('Get SteerOffset')

    def set_steer(self):
        self.send('Set Steer %s' % self.steering_entry.text())

    def set_steer_offset(self):
        self.send('Set SteerOffset %s' % self.steering_offset_entry.text())

    def set_speed(self):
        self.send('Set Speed %s' % self.speed_entry.text())

    def set_pid_params(self):
        cmd = 'Set PIDParams %s %s %s %s %s' % (
            self.Kp_entry.text(), self.Ki_entry.text(), self.Kd_entry.text(),
            self.max_integral_entry.text(), self.max_step_entry.text())

        self.send(cmd)
 
    def set_throttle(self):
        self.send('Set Throttle %s' % self.throttle_entry.text())

    def set_throttle_offset(self):
        self.send('Set ThrottleOffset %s' % self.throttle_offset_entry.text())

    def get_throttle(self):
        self.send('Get Throttle')
        self.send('Get ThrottleOffset')

    def get_pid_params(self):
        self.send('Get PIDParams\n')
    
    def update(self, line):
        self.text_area.append(line)

        if line.startswith("PIDParams"):
            p = line.split()
            self.Kp_entry.setText(p[1])
            self.Ki_entry.setText(p[2])
            self.Kd_entry.setText(p[3])
            self.max_integral_entry.setText(p[4])
            self.max_step_entry.setText(p[5])

        elif line.startswith('SteerOffset'):
            self.steering_offset_entry.setText(line.split()[1])
        elif line.startswith('Steer '):
            self.steering_entry.setText(line.split()[1])
        elif line.startswith('ThrottleOffset'):
            self.throttle_offset_entry.setText(line.split()[1])
        elif line.startswith('Throttle '):
            self.throttle_entry.setText(line.split()[1])

    def keyPressEvent(self, e):
        if e.key() == Qt.Qt.Key_Escape:
            self.close()
        
        #    if self.paused:
        #        self.paused = False
        #    else:
        #        self.paused = True
        #elif e.key() == Qt.Qt.Key_C:
        #    self.clear()

if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
   
    serial_dev = 'ttyUSB1' 
    # Qt4 doesn't support a serial port QIODevice but it does support a sockets
    server_proc = subprocess.Popen(['python', 'serial2socket.py', os.path.join('/dev', serial_dev)])

    app = Qt.QApplication(sys.argv)
    window = MainWindow(serial_dev)
    window.resize(900, 600)
    window.show()
    sys.exit(app.exec_())

