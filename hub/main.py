import sys
import os
import zipfile
import subprocess
import json
from datetime import datetime
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QPushButton, QLabel, QFileDialog, 
                             QMessageBox, QProgressBar, QFrame, QScrollArea,
                             QGridLayout, QLineEdit, QDialog, QDialogButtonBox,
                             QSplitter, QTextEdit, QTreeWidget, QTreeWidgetItem,
                             QTabWidget, QListWidget, QListWidgetItem, QComboBox,
                             QInputDialog)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSize, QRegularExpression
from PyQt6.QtGui import QFont, QColor, QIcon, QPixmap, QSyntaxHighlighter, QTextCharFormat, QTextCursor


def resource_path(relative_path):
    """Получить абсолютный путь к ресурсу, работает для dev и для PyInstaller"""
    try:
        # PyInstaller создает временную папку и сохраняет путь в _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    
    return os.path.join(base_path, relative_path)


class LuaSyntaxHighlighter(QSyntaxHighlighter):
    """Подсветка синтаксиса для Lua"""
    
    def __init__(self, parent, is_dark=True):
        super().__init__(parent)
        self.is_dark = is_dark
        self.highlighting_rules = []
        self.setup_rules()
    
    def setup_rules(self):
        self.highlighting_rules = []
        
        keywords = ["and", "break", "do", "else", "elseif", "end", "false", "for", "function", "if", 
                   "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"]
        
        if self.is_dark:
            keyword_format = QTextCharFormat()
            keyword_format.setForeground(QColor("#ff79c6"))
            
            string_format = QTextCharFormat()
            string_format.setForeground(QColor("#f1fa8c"))
            
            comment_format = QTextCharFormat()
            comment_format.setForeground(QColor("#6272a4"))
            comment_format.setFontItalic(True)
            
            number_format = QTextCharFormat()
            number_format.setForeground(QColor("#bd93f9"))
            
            function_format = QTextCharFormat()
            function_format.setForeground(QColor("#50fa7b"))
        else:
            keyword_format = QTextCharFormat()
            keyword_format.setForeground(QColor("#c000c0"))
            keyword_format.setFontWeight(QFont.Weight.Bold)
            
            string_format = QTextCharFormat()
            string_format.setForeground(QColor("#d35400"))
            string_format.setFontWeight(QFont.Weight.Bold)
            
            comment_format = QTextCharFormat()
            comment_format.setForeground(QColor("#7f8c8d"))
            comment_format.setFontItalic(True)
            
            number_format = QTextCharFormat()
            number_format.setForeground(QColor("#8e44ad"))
            number_format.setFontWeight(QFont.Weight.Bold)
            
            function_format = QTextCharFormat()
            function_format.setForeground(QColor("#27ae60"))
            function_format.setFontWeight(QFont.Weight.Bold)
        
        self.highlighting_rules.append((QRegularExpression("\\b(" + "|".join(keywords) + ")\\b"), keyword_format))
        self.highlighting_rules.append((QRegularExpression("\"[^\"]*\""), string_format))
        self.highlighting_rules.append((QRegularExpression("'[^']*'"), string_format))
        self.highlighting_rules.append((QRegularExpression("--.*$"), comment_format))
        self.highlighting_rules.append((QRegularExpression("\\b\\d+(\\.\\d+)?\\b"), number_format))
        self.highlighting_rules.append((QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\()"), function_format))
    
    def set_theme(self, is_dark):
        self.is_dark = is_dark
        self.setup_rules()
        self.rehighlight()


class ExtractThread(QThread):
    progress = pyqtSignal(int)
    finished = pyqtSignal(bool, str)
    
    def __init__(self, zip_path, extract_path):
        super().__init__()
        self.zip_path = zip_path
        self.extract_path = extract_path
        
    def run(self):
        try:
            with zipfile.ZipFile(self.zip_path, 'r') as zip_ref:
                files = zip_ref.namelist()
                for i, file in enumerate(files):
                    zip_ref.extract(file, self.extract_path)
                    progress_value = int((i + 1) / len(files) * 100)
                    self.progress.emit(progress_value)
            self.finished.emit(True, "Проект успешно создан!")
        except Exception as e:
            self.finished.emit(False, f"Ошибка: {str(e)}")


class ProjectCard(QFrame):
    open_clicked = pyqtSignal(dict)
    remove_clicked = pyqtSignal(dict)
    
    def __init__(self, project_data, parent=None):
        super().__init__(parent)
        self.project_data = project_data
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(10)
        layout.setContentsMargins(15, 15, 15, 15)
        
        self.icon_label = QLabel("📁")
        self.icon_label.setFont(QFont("Segoe UI", 48))
        self.icon_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        self.name_label = QLabel(self.project_data.get('name', 'Без названия'))
        self.name_label.setFont(QFont("Segoe UI", 14, QFont.Weight.Bold))
        self.name_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        path_text = self.project_data.get('path', '')
        if len(path_text) > 40:
            path_text = '...' + path_text[-37:]
        self.path_label = QLabel(path_text)
        self.path_label.setFont(QFont("Segoe UI", 8))
        self.path_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        modified = self.project_data.get('last_modified', 'Неизвестно')
        self.date_label = QLabel(f"📅 {modified}")
        self.date_label.setFont(QFont("Segoe UI", 8))
        self.date_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        button_layout = QHBoxLayout()
        
        self.open_btn = QPushButton("Открыть")
        self.open_btn.setFixedHeight(30)
        self.open_btn.clicked.connect(lambda: self.open_clicked.emit(self.project_data))
        
        self.remove_btn = QPushButton("🗑")
        self.remove_btn.setFixedSize(30, 30)
        self.remove_btn.clicked.connect(lambda: self.remove_clicked.emit(self.project_data))
        
        button_layout.addWidget(self.open_btn)
        button_layout.addWidget(self.remove_btn)
        
        layout.addWidget(self.icon_label)
        layout.addWidget(self.name_label)
        layout.addWidget(self.path_label)
        layout.addWidget(self.date_label)
        layout.addLayout(button_layout)
        
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.apply_style()
        
    def apply_style(self):
        self.setStyleSheet("""
            ProjectCard {
                background-color: rgba(255, 255, 255, 0.05);
                border: 1px solid rgba(255, 255, 255, 0.1);
                border-radius: 10px;
            }
            ProjectCard:hover {
                background-color: rgba(255, 255, 255, 0.1);
            }
            QPushButton:first-child {
                background-color: #4CAF50;
                border: none;
                border-radius: 5px;
                color: white;
            }
            QPushButton:first-child:hover {
                background-color: #45a049;
            }
            QPushButton:last-child {
                background-color: #f44336;
                border: none;
                border-radius: 5px;
                color: white;
            }
            QPushButton:last-child:hover {
                background-color: #da190b;
            }
        """)


class CreateProjectDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Создать новый проект")
        self.setFixedSize(500, 250)
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(15)
        
        title = QLabel("Создание нового проекта Blazebolt")
        title.setFont(QFont("Segoe UI", 16, QFont.Weight.Bold))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        # Имя проекта
        name_layout = QHBoxLayout()
        name_label = QLabel("Имя проекта:")
        name_label.setFixedWidth(100)
        self.name_input = QLineEdit()
        self.name_input.setPlaceholderText("Введите название проекта")
        name_layout.addWidget(name_label)
        name_layout.addWidget(self.name_input)
        
        # Путь к проекту
        path_layout = QHBoxLayout()
        path_label = QLabel("Путь:")
        path_label.setFixedWidth(100)
        self.path_input = QLineEdit()
        self.path_input.setPlaceholderText("Выберите папку для проекта")
        self.path_input.setReadOnly(True)
        self.browse_btn = QPushButton("Обзор...")
        self.browse_btn.clicked.connect(self.browse_folder)
        path_layout.addWidget(path_label)
        path_layout.addWidget(self.path_input)
        path_layout.addWidget(self.browse_btn)
        
        # Кнопки
        buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | 
                                   QDialogButtonBox.StandardButton.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        
        layout.addWidget(title)
        layout.addSpacing(20)
        layout.addLayout(name_layout)
        layout.addLayout(path_layout)
        layout.addStretch()
        layout.addWidget(buttons)
        
        self.setStyleSheet("""
            QDialog { background-color: #2d2d2d; }
            QLabel { color: white; }
            QLineEdit {
                background-color: #3c3c3c;
                border: 1px solid #555;
                border-radius: 5px;
                padding: 5px;
                color: white;
            }
            QPushButton {
                background-color: #4CAF50;
                border: none;
                border-radius: 5px;
                padding: 5px 15px;
                color: white;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
        """)
        
    def browse_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "Выберите папку для проекта")
        if folder:
            # Нормализуем путь
            folder = os.path.normpath(folder)
            self.path_input.setText(folder)
            
    def get_project_data(self):
        path = os.path.join(self.path_input.text(), self.name_input.text())
        path = os.path.normpath(path)
        return {
            'name': self.name_input.text(),
            'path': path
        }


class BlazeboltHub(QMainWindow):
    def __init__(self):
        super().__init__()
        self.projects = []
        self.current_theme = "dark"
        self.init_ui()
        self.load_projects()
        self.apply_theme()
        
    def init_ui(self):
        self.setWindowTitle("Blazebolt Hub")
        self.setMinimumSize(1200, 700)
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        main_layout = QVBoxLayout(central_widget)
        main_layout.setSpacing(0)
        main_layout.setContentsMargins(0, 0, 0, 0)
        
        # Верхняя панель
        top_bar = self.create_top_bar()
        main_layout.addWidget(top_bar)
        
        # Контент
        self.projects_widget = self.create_projects_page()
        main_layout.addWidget(self.projects_widget)
        
        # Редактор (скрыт по умолчанию)
        self.editor_widget = self.create_editor_page()
        self.editor_widget.hide()
        main_layout.addWidget(self.editor_widget)
        
        self.status_bar = self.statusBar()
        self.status_bar.showMessage("Готов к работе")
        
    def create_top_bar(self):
        top_bar = QFrame()
        top_bar.setFixedHeight(70)
        
        layout = QHBoxLayout(top_bar)
        layout.setContentsMargins(20, 10, 20, 10)
        
        logo = QLabel("⚡ BLAZEBOLT")
        logo.setFont(QFont("Segoe UI", 18, QFont.Weight.Bold))
        
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("🔍 Поиск проектов...")
        self.search_input.setFixedWidth(250)
        self.search_input.textChanged.connect(self.search_projects)
        
        self.new_btn = QPushButton("➕ Новый проект")
        self.new_btn.setFixedSize(130, 40)
        self.new_btn.clicked.connect(self.create_new_project)
        
        self.theme_btn = QPushButton("🌙")
        self.theme_btn.setFixedSize(40, 40)
        self.theme_btn.clicked.connect(self.toggle_theme)
        
        layout.addWidget(logo)
        layout.addStretch()
        layout.addWidget(self.search_input)
        layout.addSpacing(20)
        layout.addWidget(self.new_btn)
        layout.addSpacing(10)
        layout.addWidget(self.theme_btn)
        
        return top_bar
    
    def create_editor_page(self):
        widget = QWidget()
        
        # Основной layout по вертикали
        main_layout = QVBoxLayout(widget)
        main_layout.setSpacing(0)
        main_layout.setContentsMargins(0, 0, 0, 0)
        
        # === Верхняя панель ===
        top_panel = QFrame()
        top_panel.setFixedHeight(50)
        top_layout = QHBoxLayout(top_panel)
        top_layout.setContentsMargins(10, 5, 10, 5)
        
        back_btn = QPushButton("← Назад")
        back_btn.setFixedSize(100, 35)
        back_btn.clicked.connect(self.close_editor)
        top_layout.addWidget(back_btn)
        
        top_layout.addStretch()
        
        self.project_name_label = QLabel("Проект")
        self.project_name_label.setFont(QFont("Segoe UI", 14, QFont.Weight.Bold))
        top_layout.addWidget(self.project_name_label)
        
        top_layout.addStretch()
        
        play_btn = QPushButton("▶ Запустить")
        play_btn.setFixedSize(120, 35)
        play_btn.setStyleSheet("""
            QPushButton {
                background-color: #4CAF50;
                color: white;
                border: none;
                border-radius: 5px;
                font-weight: bold;
            }
            QPushButton:hover { background-color: #45a049; }
        """)
        play_btn.clicked.connect(self.run_project)
        top_layout.addWidget(play_btn)
        
        main_layout.addWidget(top_panel)
        
        # === Центральная часть ===
        center_splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # === Левая панель: Файлы + Scene Objects ===
        left_panel = QFrame()
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(5, 5, 5, 5)
        
        # Файлы
        files_header = QLabel("📁 Файлы проекта")
        files_header.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        files_header.setToolTip("Двойной клик - открыть файл")
        left_layout.addWidget(files_header)
        
        self.file_tree = QTreeWidget()
        self.file_tree.setHeaderLabel("Файлы")
        self.file_tree.setFixedHeight(200)
        self.file_tree.itemDoubleClicked.connect(self.on_file_double_clicked)
        left_layout.addWidget(self.file_tree)
        
        # Кнопки файлов
        file_buttons = QHBoxLayout()
        new_folder_btn = QPushButton("📁+")
        new_folder_btn.setFixedSize(35, 30)
        new_folder_btn.setToolTip("Создать папку")
        new_folder_btn.clicked.connect(self.create_folder)
        file_buttons.addWidget(new_folder_btn)
        
        new_file_btn = QPushButton("📄+")
        new_file_btn.setFixedSize(35, 30)
        new_file_btn.setToolTip("Создать файл")
        new_file_btn.clicked.connect(self.create_file)
        file_buttons.addWidget(new_file_btn)
        
        del_btn = QPushButton("🗑")
        del_btn.setFixedSize(35, 30)
        del_btn.setToolTip("Удалить выбранное")
        del_btn.clicked.connect(self.delete_selected)
        file_buttons.addWidget(del_btn)
        
        file_buttons.addStretch()
        left_layout.addLayout(file_buttons)
        
        # Scene Objects
        scene_header = QLabel("🎯 Объекты сцены")
        scene_header.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        left_layout.addWidget(scene_header)
        
        self.objects_list = QListWidget()
        self.objects_list.setAlternatingRowColors(True)
        self.objects_list.itemClicked.connect(self.on_object_clicked)
        left_layout.addWidget(self.objects_list)
        
        # Кнопки объектов
        obj_buttons = QHBoxLayout()
        add_obj_btn = QPushButton("➕")
        add_obj_btn.setFixedSize(30, 30)
        add_obj_btn.setToolTip("Добавить объект")
        add_obj_btn.clicked.connect(self.add_scene_object)
        obj_buttons.addWidget(add_obj_btn)
        
        del_obj_btn = QPushButton("🗑")
        del_obj_btn.setFixedSize(30, 30)
        del_obj_btn.setToolTip("Удалить объект")
        del_obj_btn.clicked.connect(self.delete_scene_object)
        obj_buttons.addWidget(del_obj_btn)
        
        obj_buttons.addStretch()
        left_layout.addLayout(obj_buttons)
        
        # === Центральная панель: Inspector + Code ===
        center_layout = QVBoxLayout()
        
        # Inspector
        inspector_header = QLabel("📋 Inspector")
        inspector_header.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        inspector_header.setToolTip("Свойства выбранного объекта сцены")
        center_layout.addWidget(inspector_header)
        
        # Свойства объекта
        props_frame = QWidget()
        props_layout = QGridLayout(props_frame)
        
        props_layout.addWidget(QLabel("Имя:"), 0, 0)
        self.prop_name = QLineEdit()
        self.prop_name.setPlaceholderText("имя объекта")
        props_layout.addWidget(self.prop_name, 0, 1)
        
        props_layout.addWidget(QLabel("Тип:"), 1, 0)
        self.prop_type = QComboBox()
        self.prop_type.addItems(["Sprite2D", "Node2D", "Label", "Audio", "Camera2D", "Area2D", "RigidBody2D", "StaticBody2D", "CharacterBody2D", "Marker2D"])
        props_layout.addWidget(self.prop_type, 1, 1)
        
        props_layout.addWidget(QLabel("X:"), 2, 0)
        self.prop_x = QLineEdit("0")
        self.prop_x.setFixedWidth(80)
        props_layout.addWidget(self.prop_x, 2, 1)
        
        props_layout.addWidget(QLabel("Y:"), 3, 0)
        self.prop_y = QLineEdit("0")
        self.prop_y.setFixedWidth(80)
        props_layout.addWidget(self.prop_y, 3, 1)
        
        props_layout.addWidget(QLabel("Ширина:"), 4, 0)
        self.prop_width = QLineEdit("0.1")
        self.prop_width.setFixedWidth(80)
        props_layout.addWidget(self.prop_width, 4, 1)
        
        props_layout.addWidget(QLabel("Высота:"), 5, 0)
        self.prop_height = QLineEdit("0.1")
        self.prop_height.setFixedWidth(80)
        props_layout.addWidget(self.prop_height, 5, 1)
        
        props_layout.addWidget(QLabel("Текстура:"), 6, 0)
        self.prop_texture = QLineEdit()
        self.prop_texture.setPlaceholderText("путь к текстуре")
        props_layout.addWidget(self.prop_texture, 6, 1)
        
        props_layout.addWidget(QLabel("Скрипт:"), 7, 0)
        self.prop_script = QLineEdit()
        self.prop_script.setPlaceholderText("путь к скрипту")
        props_layout.addWidget(self.prop_script, 7, 1)
        
        apply_btn = QPushButton("✅ Применить")
        apply_btn.clicked.connect(self.apply_object_properties)
        props_layout.addWidget(apply_btn, 8, 0, 1, 2)
        
        center_layout.addWidget(props_frame)
        
        # Разделитель
        center_layout.addWidget(QLabel("📝 Редактор кода"))
        
        # Code Editor
        self.code_editor = QTextEdit()
        self.code_editor.setFont(QFont("Consolas", 11))
        self.code_highlighter = LuaSyntaxHighlighter(self.code_editor.document(), True)
        self.code_editor.setPlainText("""-- Скрипт объекта
-- Создайте объект и прикрепите этот скрипт

function _ready()
    print("Объект готов!")
end

function _process(dt)
    -- Вызывается каждый кадр
end
""")
        center_layout.addWidget(self.code_editor)
        
        center_widget = QWidget()
        center_widget.setLayout(center_layout)

        # Правая панель - Scene Tree
        right_panel = QFrame()
        right_panel.setFixedWidth(250)
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(5, 5, 5, 5)
        
        right_layout.addWidget(QLabel("Сцена:"))
        self.scene_name_edit = QLineEdit("main")
        self.scene_name_edit.setPlaceholderText("имя файла сцены .json")
        right_layout.addWidget(self.scene_name_edit)
        
        save_scene_btn = QPushButton("💾 Сохранить сцену")
        save_scene_btn.clicked.connect(self.save_scene)
        right_layout.addWidget(save_scene_btn)
        
        load_scene_btn = QPushButton("📂 Загрузить сцену")
        load_scene_btn.clicked.connect(self.load_scene)
        right_layout.addWidget(load_scene_btn)
        
        right_layout.addStretch()
        
        # Собираем together
        center_splitter.addWidget(left_panel)
        center_splitter.addWidget(center_widget)
        center_splitter.addWidget(right_panel)
        
        center_splitter.setSizes([250, 500, 200])
        
        main_layout.addWidget(center_splitter)
        
        # Стили
        widget.setStyleSheet("""
            QFrame { background-color: #1e1e2e; }
            QTreeWidget, QListWidget { 
                background-color: #2a2a3d; 
                color: #e0e0f0; 
                border: 1px solid #3a3a55;
            }
            QLineEdit, QComboBox {
                background-color: #151525;
                color: #e0e0f0;
                border: 1px solid #3a3a55;
                padding: 5px;
                border-radius: 3px;
            }
            QPushButton {
                background-color: #3a3a55;
                color: #fff;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
            }
            QPushButton:hover { background-color: #4a4a6a; }
            QLabel { color: #e0e0f0; }
            QTabWidget::pane { border: 1px solid #3a3a55; }
            QTabBar::tab { 
                background-color: #2a2a3d; 
                color: #888; 
                padding: 8px 16px; 
            }
        """)
        
        return widget
        inspector_area = QScrollArea()
        inspector_area.setWidgetResizable(True)
        inspector_content = QWidget()
        inspector_layout = QVBoxLayout(inspector_content)
        
        inspector_header = QLabel("📋 Inspector")
        inspector_header.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        inspector_layout.addWidget(inspector_header)
        
        # Свойства Node
        node_section = QLabel("Node")
        node_section.setFont(QFont("Segoe UI", 10, QFont.Weight.Bold))
        node_section.setStyleSheet("color: #6c63ff;")
        inspector_layout.addWidget(node_section)
        
        self.node_name_label = QLabel("Name: (selected)")
        inspector_layout.addWidget(self.node_name_label)
        
        self.node_type_label = QLabel("Type: Node2D")
        inspector_layout.addWidget(self.node_type_label)
        
        # Transform
        transform_section = QLabel("Transform")
        transform_section.setFont(QFont("Segoe UI", 10, QFont.Weight.Bold))
        transform_section.setStyleSheet("color: #6c63ff;")
        inspector_layout.addWidget(transform_section)
        
        self.pos_label = QLabel("Position X: 0, Y: 0")
        inspector_layout.addWidget(self.pos_label)
        
        self.rotation_label = QLabel("Rotation: 0")
        inspector_layout.addWidget(self.rotation_label)
        
        self.scale_label = QLabel("Scale: 1 x 1")
        inspector_layout.addWidget(self.scale_label)
        
        # Visibility
        vis_section = QLabel("Visibility")
        vis_section.setFont(QFont("Segoe UI", 10, QFont.Weight.Bold))
        vis_section.setStyleSheet("color: #6c63ff;")
        inspector_layout.addWidget(vis_section)
        
        self.visible_label = QLabel("Visible: ✓")
        inspector_layout.addWidget(self.visible_label)
        
        inspector_layout.addStretch()
        
        inspector_area.setWidget(inspector_content)
        
        # Вкладка Code Editor
        code_tab = QWidget()
        code_layout = QVBoxLayout(code_tab)
        
        code_header = QHBoxLayout()
        code_title = QLabel("📝 Редактор кода")
        code_title.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        code_header.addWidget(code_title)
        code_header.addStretch()
        
        self.save_code_btn = QPushButton("💾 Сохранить")
        self.save_code_btn.setFixedSize(100, 30)
        code_header.addWidget(self.save_code_btn)
        
        code_layout.addLayout(code_header)
        
        self.code_editor = QTextEdit()
        self.code_editor.setFont(QFont("Consolas", 11))
        is_dark = self.current_theme == "dark"
        self.code_highlighter = LuaSyntaxHighlighter(self.code_editor.document(), is_dark)
        self.code_editor.setPlainText("-- Редактор кода\n-- Выберите файл для редактирования\n\nfunction _ready()\n    print(\"Hello, BlazeBolt!\")\nend\n\nfunction _process(dt)\n    -- Обновление каждый кадр\nend")
        code_layout.addWidget(self.code_editor)
        
        center_tabs.addTab(code_tab, "Code")
        
        right_header = QLabel("🌳 Scene")
        right_header.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        right_layout.addWidget(right_header)
        
        self.scene_tree = QTreeWidget()
        self.scene_tree.setHeaderLabel("Сцена")
        self.scene_tree.itemClicked.connect(self.on_scene_item_clicked)
        right_layout.addWidget(self.scene_tree)
        
        # Добавить корневой узел
        root_item = QTreeWidgetItem(["Root (Node2D)"])
        root_item.setText(0, "[Root]")
        self.scene_tree.addTopLevelItem(root_item)
        
        # Кнопки управления сценой
        scene_buttons = QHBoxLayout()
        
        add_node_btn = QPushButton("➕")
        add_node_btn.setFixedSize(30, 30)
        add_node_btn.setToolTip("Добавить узел")
        scene_buttons.addWidget(add_node_btn)
        
        del_node_btn = QPushButton("🗑")
        del_node_btn.setFixedSize(30, 30)
        del_node_btn.setToolTip("Удалить узел")
        scene_buttons.addWidget(del_node_btn)
        
        scene_buttons.addStretch()
        
        right_layout.addLayout(scene_buttons)
        
        # Нижняя панель - Output
        output_panel = QFrame()
        output_panel.setFixedHeight(150)
        output_layout = QVBoxLayout(output_panel)
        output_layout.setContentsMargins(5, 5, 5, 5)
        
        # Собираем редактор
        main_splitter.addWidget(left_panel)
        main_splitter.addWidget(center_tabs)
        main_splitter.addWidget(right_panel)
        
        main_splitter.setSizes([250, 700, 250])
        
        layout = QVBoxLayout(widget)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)
        
        # Верхняя панель редактора
        editor_top = QFrame()
        editor_top.setFixedHeight(50)
        editor_top_layout = QHBoxLayout(editor_top)
        
        back_btn = QPushButton("← Назад")
        back_btn.setFixedSize(100, 35)
        back_btn.clicked.connect(self.close_editor)
        editor_top_layout.addWidget(back_btn)
        
        editor_top_layout.addStretch()
        
        self.project_name_label = QLabel("Проект")
        self.project_name_label.setFont(QFont("Segoe UI", 14, QFont.Weight.Bold))
        editor_top_layout.addWidget(self.project_name_label)
        
        editor_top_layout.addStretch()
        
        play_btn = QPushButton("▶ Запустить")
        play_btn.setFixedSize(120, 35)
        play_btn.setStyleSheet("background-color: #4CAF50; color: white; border: none; border-radius: 5px;")
        editor_top_layout.addWidget(play_btn)
        
        layout.addWidget(editor_top)
        layout.addWidget(main_splitter)
        layout.addWidget(output_panel)
        
        # Применяем стили
        widget.setStyleSheet("""
            QFrame { background-color: #1e1e2e; }
            QTreeWidget { background-color: #2a2a3d; color: #e0e0f0; border: none; }
            QTreeWidget::item:selected { background-color: #4a4a6a; }
            QTabWidget::pane { border: 1px solid #3a3a55; background-color: #1e1e2e; }
            QTabBar::tab { background-color: #2a2a3d; color: #888; padding: 8px 16px; border: none; }
            QTabBar::tab:selected { background-color: #3a3a55; color: #fff; }
            QListWidget { background-color: #151525; color: #e0e0f0; border: none; }
            QPushButton { background-color: #3a3a55; color: #fff; border: none; padding: 8px 16px; border-radius: 4px; }
            QPushButton:hover { background-color: #4a4a6a; }
        """)
        
        return widget
    
    def on_file_double_clicked(self, item, column):
        """Обработчик двойного клика по файлу"""
        path = item.data(0, 1)
        if path and os.path.isfile(path):
            ext = os.path.splitext(path)[1].lower()
            if ext == '.json':
                self.load_scene_from_file(path)
            elif ext in ['.lua', '.txt', '.md']:
                try:
                    with open(path, 'r', encoding='utf-8') as f:
                        content = f.read()
                    self.code_editor.setPlainText(content)
                    self.scene_current_file = path
                except Exception as e:
                    pass
    
    def load_scene_from_file(self, scene_file):
        """Загрузить сцену из файла"""
        if not os.path.exists(scene_file):
            return
        try:
            with open(scene_file, 'r', encoding='utf-8') as f:
                scene_data = json.load(f)
            self.scene_objects = scene_data.get("objects", [])
            self.objects_list.clear()
            for obj in self.scene_objects:
                self.objects_list.addItem(f"{obj['name']} ({obj['type']})")
            self.scene_current_file = scene_file
        except Exception as e:
            pass
    
    def on_scene_item_clicked(self, item, column):
        """Обработчик клика по элементу сцены"""
        name = item.text(0)
        self.node_name_label.setText(f"Name: {name}")
    
    def open_editor(self, project_data):
        """Открыть редактор проекта"""
        self.current_project = project_data
        self.project_name_label.setText(project_data.get("name", "Проект"))
        self.scene_objects = []
        self.objects_list.clear()
        
        self.load_project_files(project_data.get("path", ""))
        
        self.projects_widget.hide()
        self.editor_widget.show()
        self.apply_theme()
    
    def close_editor(self):
        """Закрыть редактор"""
        self.editor_widget.hide()
        self.projects_widget.show()
    
    def run_project(self):
        """Запустить проект (game.exe)"""
        project_path = self.current_project.get("path", "")
        if project_path:
            game_exe = os.path.join(project_path, "game.exe")
            if os.path.exists(game_exe):
                try:
                    subprocess.Popen([game_exe], cwd=project_path)
                except Exception as e:
                    pass
            else:
                pass
    
    def add_scene_object(self):
        """Добавить новый объект на сцену"""
        import random
        idx = len(self.scene_objects)
        
        base_x = (idx % 5) * 160 - 320
        base_y = (idx // 5) * 120 - 240
        
        obj_types = ["Sprite2D", "Label", "Camera2D", "Area2D", "Node2D"]
        textures = ["player.png", "enemy.png", "coin.png", "background.png", ""]
        
        name = f"Object_{idx + 1}"
        obj = {
            "name": name,
            "type": obj_types[idx % len(obj_types)],
            "x": base_x + random.randint(-20, 20),
            "y": base_y + random.randint(-20, 20),
            "width": random.choice([32, 48, 64, 96, 128]),
            "height": random.choice([32, 48, 64, 96, 128]),
            "texture": random.choice(textures),
            "script": ""
        }
        self.scene_objects.append(obj)
        self.objects_list.addItem(f"{name} ({obj['type']})")
    
    def delete_scene_object(self):
        """Удалить выбранный объект"""
        current_row = self.objects_list.currentRow()
        if current_row >= 0:
            name = self.scene_objects[current_row]["name"]
            self.scene_objects.pop(current_row)
            self.objects_list.takeItem(current_row)
    
    def create_folder(self):
        """Создать папку"""
        project_path = self.current_project.get("path", "")
        if not project_path:
            return
        name, ok = QInputDialog.getText(self, "Новая папка", "Имя папки:")
        if ok and name:
            folder_path = os.path.join(project_path, name)
            try:
                os.makedirs(folder_path, exist_ok=True)
                self.load_project_files(project_path)
            except Exception as e:
                pass
    
    def create_file(self):
        """Создать файл"""
        project_path = self.current_project.get("path", "")
        if not project_path:
            return
        name, ok = QInputDialog.getText(self, "Новый файл", "Имя файла:")
        if ok and name:
            file_path = os.path.join(project_path, name)
            try:
                if not os.path.exists(file_path):
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write("")
                    self.load_project_files(project_path)
                else:
                    pass
            except Exception as e:
                pass
    
    def delete_selected(self):
        """Удалить выбранный файл/папку"""
        item = self.file_tree.currentItem()
        if not item:
            return
        path = item.data(0, 1)
        if not path or not os.path.exists(path):
            return
        name = os.path.basename(path)
        reply = QMessageBox.question(self, "Удалить", f"Удалить {name}?",
                                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        if reply == QMessageBox.StandardButton.Yes:
            try:
                if os.path.isfile(path):
                    os.remove(path)
                elif os.path.isdir(path):
                    import shutil
                    shutil.rmtree(path)
                project_path = self.current_project.get("path", "")
                self.load_project_files(project_path)
            except Exception as e:
                pass
    
    def on_object_clicked(self, item):
        """Выбрать объект для редактирования"""
        row = self.objects_list.row(item)
        if row >= 0 and row < len(self.scene_objects):
            self.current_object_index = row
            obj = self.scene_objects[row]
            self.prop_name.setText(obj["name"])
            self.prop_type.setCurrentText(obj["type"])
            self.prop_x.setText(str(obj["x"]))
            self.prop_y.setText(str(obj["y"]))
            self.prop_width.setText(str(obj["width"]))
            self.prop_height.setText(str(obj["height"]))
            self.prop_texture.setText(obj["texture"])
            self.prop_script.setText(obj["script"])
            # f"[Scene] Выбран: {obj['name']}")
    
    def apply_object_properties(self):
        """Применить изменения к объекту"""
        if self.current_object_index >= 0 and self.current_object_index < len(self.scene_objects):
            obj = self.scene_objects[self.current_object_index]
            obj["name"] = self.prop_name.text()
            obj["type"] = self.prop_type.currentText()
            obj["x"] = float(self.prop_x.text() or "0")
            obj["y"] = float(self.prop_y.text() or "0")
            obj["width"] = float(self.prop_width.text() or "0.1")
            obj["height"] = float(self.prop_height.text() or "0.1")
            obj["texture"] = self.prop_texture.text()
            obj["script"] = self.prop_script.text()
            
            # Обновить в списке
            self.objects_list.item(self.current_object_index).setText(f"{obj['name']} ({obj['type']})")
    
    def save_scene(self):
        """Сохранить сцену в JSON"""
        if not hasattr(self, 'scene_objects'):
            self.scene_objects = []
        
        scene_name = self.scene_name_edit.text() or "main"
        scene_data = {
            "name": scene_name,
            "objects": self.scene_objects
        }
        
        project_path = self.current_project.get("path", "")
        if not project_path:
            # "[Error] Проект не выбран")
            return
        
        # Сохраняем в папку scenes
        scenes_dir = os.path.join(project_path, "scenes")
        os.makedirs(scenes_dir, exist_ok=True)
        
        scene_file = os.path.join(scenes_dir, f"{scene_name}.json")
        try:
            import json
            with open(scene_file, 'w', encoding='utf-8') as f:
                json.dump(scene_data, f, indent=4, ensure_ascii=False)
        except Exception as e:
            pass
    
    def load_scene(self):
        """Загрузить сцену из JSON"""
        project_path = self.current_project.get("path", "")
        if not project_path:
            return
        
        scene_name = self.scene_name_edit.text() or "main"
        scene_file = os.path.join(project_path, "scenes", f"{scene_name}.json")
        
        if not os.path.exists(scene_file):
            return
        
        try:
            import json
            with open(scene_file, 'r', encoding='utf-8') as f:
                scene_data = json.load(f)
            
            self.scene_objects = scene_data.get("objects", [])
            self.objects_list.clear()
            for obj in self.scene_objects:
                self.objects_list.addItem(f"{obj['name']} ({obj['type']})")
        
        except Exception as e:
            pass
    
    def load_project_files(self, project_path):
        """Загрузить файлы проекта в дерево"""
        self.file_tree.clear()
        
        if not project_path or not os.path.exists(project_path):
            return
        
        # Рекурсивно добавляем файлы
        def add_tree_items(parent, path):
            try:
                for item in os.listdir(path):
                    item_path = os.path.join(path, item)
                    tree_item = QTreeWidgetItem([item])
                    tree_item.setData(0, 1, item_path)
                    parent.addChild(tree_item)
                    if os.path.isdir(item_path):
                        add_tree_items(tree_item, item_path)
            except PermissionError:
                pass
        
        root_item = QTreeWidgetItem([os.path.basename(project_path)])
        self.file_tree.addTopLevelItem(root_item)
        add_tree_items(root_item, project_path)
        root_item.setExpanded(True)
        
    def create_projects_page(self):
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(20)
        layout.setContentsMargins(30, 30, 30, 30)
        
        header = QLabel("Мои проекты")
        header.setFont(QFont("Segoe UI", 24, QFont.Weight.Bold))
        
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setFrameShape(QFrame.Shape.NoFrame)
        
        self.projects_container = QWidget()
        self.projects_grid = QGridLayout(self.projects_container)
        self.projects_grid.setSpacing(20)
        
        scroll_area.setWidget(self.projects_container)
        layout.addWidget(header)
        layout.addWidget(scroll_area)
        
        return widget
        
    def load_projects(self):
        try:
            # Получаем путь для сохранения файла с проектами
            projects_file = resource_path('blazebolt_projects.json')
            
            # Если файл находится внутри exe (нельзя писать), используем другой путь
            if hasattr(sys, '_MEIPASS'):
                # Запущено из exe - сохраняем в папке пользователя
                app_data = os.path.join(os.environ.get('APPDATA', os.path.expanduser('~')), 'BlazeboltHub')
                os.makedirs(app_data, exist_ok=True)
                projects_file = os.path.join(app_data, 'blazebolt_projects.json')
            
            if os.path.exists(projects_file):
                with open(projects_file, 'r', encoding='utf-8') as f:
                    self.projects = json.load(f)
            else:
                self.projects = []
                self.save_projects()
        except Exception as e:
            print(f"Ошибка загрузки: {e}")
            self.projects = []
        self.display_projects()
        
    def save_projects(self):
        try:
            # Получаем путь для сохранения файла с проектами
            if hasattr(sys, '_MEIPASS'):
                # Запущено из exe - сохраняем в папке пользователя
                app_data = os.path.join(os.environ.get('APPDATA', os.path.expanduser('~')), 'BlazeboltHub')
                os.makedirs(app_data, exist_ok=True)
                projects_file = os.path.join(app_data, 'blazebolt_projects.json')
            else:
                projects_file = 'blazebolt_projects.json'
            
            with open(projects_file, 'w', encoding='utf-8') as f:
                json.dump(self.projects, f, ensure_ascii=False, indent=2)
        except Exception as e:
            print(f"Ошибка сохранения: {e}")
            
    def display_projects(self):
        # Очищаем сетку
        while self.projects_grid.count():
            item = self.projects_grid.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
                
        # Отображаем проекты
        for i, project in enumerate(self.projects):
            card = ProjectCard(project)
            card.open_clicked.connect(self.open_project)
            card.remove_clicked.connect(self.remove_project)
            row = i // 3
            col = i % 3
            self.projects_grid.addWidget(card, row, col)
            
        # Карточка нового проекта
        new_card = self.create_new_project_card()
        row = len(self.projects) // 3
        col = len(self.projects) % 3
        self.projects_grid.addWidget(new_card, row, col)
        
    def create_new_project_card(self):
        card = QFrame()
        card.setStyleSheet("""
            QFrame {
                background-color: rgba(76, 175, 80, 0.1);
                border: 2px dashed #4CAF50;
                border-radius: 10px;
            }
            QFrame:hover {
                background-color: rgba(76, 175, 80, 0.2);
            }
        """)
        card.setCursor(Qt.CursorShape.PointingHandCursor)
        
        layout = QVBoxLayout(card)
        layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        plus = QLabel("➕")
        plus.setFont(QFont("Segoe UI", 48))
        plus.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        text = QLabel("Новый проект")
        text.setFont(QFont("Segoe UI", 14))
        text.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        layout.addWidget(plus)
        layout.addWidget(text)
        
        card.mousePressEvent = lambda e: self.create_new_project()
        
        return card
        
    def search_projects(self):
        search_text = self.search_input.text().lower()
        for i in range(self.projects_grid.count()):
            item = self.projects_grid.itemAt(i)
            if item and item.widget() and isinstance(item.widget(), ProjectCard):
                name = item.widget().project_data.get('name', '').lower()
                item.widget().setVisible(search_text in name)
                
    def create_new_project(self):
        dialog = CreateProjectDialog(self)
        if dialog.exec() == QDialog.DialogCode.Accepted:
            data = dialog.get_project_data()
            
            if not data['name'] or not data['path']:
                QMessageBox.warning(self, "Ошибка", "Заполните все поля")
                return
                
            try:
                # Создаем папку проекта
                os.makedirs(data['path'], exist_ok=True)
                
                # Используем resource_path для поиска void.zip
                zip_path = resource_path("void.zip")
                
                if os.path.exists(zip_path):
                    # Показываем диалог с прогрессом
                    self.progress_dialog = QDialog(self)
                    self.progress_dialog.setWindowTitle("Создание проекта")
                    self.progress_dialog.setFixedSize(400, 150)
                    self.progress_dialog.setModal(True)
                    
                    layout = QVBoxLayout(self.progress_dialog)
                    layout.addWidget(QLabel("Распаковка void.zip..."))
                    
                    progress_bar = QProgressBar()
                    layout.addWidget(progress_bar)
                    
                    self.progress_dialog.show()
                    
                    # Запускаем распаковку в потоке
                    self.extract_thread = ExtractThread(zip_path, data['path'])
                    self.extract_thread.progress.connect(progress_bar.setValue)
                    self.extract_thread.finished.connect(lambda success, msg: self.on_extract_finished(success, msg, data))
                    self.extract_thread.start()
                else:
                    QMessageBox.warning(self, "Предупреждение", 
                        f"Файл void.zip не найден по пути:\n{zip_path}\nПапка проекта создана, но архив не распакован.")
                    
                    # Всё равно сохраняем проект
                    project_info = {
                        'name': data['name'],
                        'path': data['path'],
                        'created': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                        'last_modified': datetime.now().strftime('%Y-%m-%d')
                    }
                    self.projects.append(project_info)
                    self.save_projects()
                    self.display_projects()
                
            except Exception as e:
                QMessageBox.critical(self, "Ошибка", f"Не удалось создать проект: {str(e)}")
    
    def on_extract_finished(self, success, message, project_data):
        """Обработка завершения распаковки"""
        self.progress_dialog.close()
        
        if success:
            # Сохраняем проект
            project_info = {
                'name': project_data['name'],
                'path': project_data['path'],
                'created': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                'last_modified': datetime.now().strftime('%Y-%m-%d')
            }
            
            self.projects.append(project_info)
            self.save_projects()
            self.display_projects()
            
            QMessageBox.information(self, "Успех", message)
        else:
            QMessageBox.critical(self, "Ошибка", message)
            
    def open_project(self, project_data):
        """Открытие редактора проекта"""
        self.open_editor(project_data)
            
    def remove_project(self, project_data):
        reply = QMessageBox.question(self, "Удаление", 
                                    f"Удалить '{project_data.get('name')}' из списка?\n(Файлы проекта останутся на диске)",
                                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        if reply == QMessageBox.StandardButton.Yes:
            self.projects = [p for p in self.projects if p.get('path') != project_data.get('path')]
            self.save_projects()
            self.display_projects()
            self.status_bar.showMessage(f"Проект '{project_data.get('name')}' удалён из списка")
            
    def apply_theme(self):
        if self.current_theme == "dark":
            self.setStyleSheet("""
                QMainWindow, QWidget { background-color: #1e1e2e; color: #ffffff; }
                QPushButton {
                    background-color: #313244;
                    border: none;
                    border-radius: 8px;
                    padding: 8px;
                    font-weight: bold;
                    color: white;
                }
                QPushButton:hover { background-color: #45475a; }
                QLineEdit {
                    background-color: #313244;
                    border: 1px solid #45475a;
                    border-radius: 8px;
                    padding: 8px;
                    color: white;
                }
                QScrollArea { background-color: #1e1e2e; border: none; }
                QListWidget, QTreeWidget { background-color: #2a2a3d; color: #e0e0f0; border: none; }
                QTextEdit { background-color: #1e1e2e; color: #e0e0f0; border: none; }
                QLabel { color: #ffffff; }
                QComboBox {
                    background-color: #313244;
                    color: white;
                    border: 1px solid #45475a;
                    border-radius: 8px;
                    padding: 8px;
                }
            """)
            self.theme_btn.setText("🌙")
        else:
            self.setStyleSheet("""
                QMainWindow, QWidget { background-color: #f0f4f8; color: #2c3e50; }
                QPushButton {
                    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3498db, stop:1 #2980b9);
                    border: none;
                    border-radius: 6px;
                    padding: 10px 16px;
                    font-weight: bold;
                    color: white;
                }
                QPushButton:hover {
                    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5dade2, stop:1 #3498db);
                }
                QLineEdit {
                    background-color: white;
                    border: 2px solid #bdc3c7;
                    border-radius: 6px;
                    padding: 10px;
                    color: #2c3e50;
                }
                QLineEdit:focus {
                    border-color: #3498db;
                }
                QScrollArea { background-color: #f0f4f8; border: none; }
                QListWidget, QTreeWidget {
                    background-color: white;
                    color: #2c3e50;
                    border: 1px solid #ddd;
                    border-radius: 6px;
                    outline: none;
                }
                QListWidget::item:selected, QTreeWidget::item:selected {
                    background-color: #3498db;
                    color: white;
                }
                QListWidget::item:hover, QTreeWidget::item:hover {
                    background-color: #ecf0f1;
                }
                QTextEdit {
                    background-color: white;
                    color: #2c3e50;
                    border: 1px solid #ddd;
                    border-radius: 6px;
                    padding: 8px;
                }
                QLabel { color: #2c3e50; }
                QComboBox {
                    background-color: white;
                    color: #2c3e50;
                    border: 2px solid #bdc3c7;
                    border-radius: 6px;
                    padding: 8px;
                }
                QComboBox:hover {
                    border-color: #3498db;
                }
                QFrame {
                    background-color: white;
                    border-radius: 8px;
                    border: 1px solid #e0e0e0;
                }
                QSplitter::handle {
                    background-color: #bdc3c7;
                }
            """)
            self.theme_btn.setText("☀️")
        
        if hasattr(self, 'code_editor'):
            self.code_editor.setFont(QFont("Consolas", 11))
        if hasattr(self, 'code_highlighter'):
            if self.current_theme == "dark":
                self.code_highlighter.set_theme(True)
            else:
                self.code_highlighter.set_theme(False)
            
    def toggle_theme(self):
        self.current_theme = "light" if self.current_theme == "dark" else "dark"
        self.apply_theme()


def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    window = BlazeboltHub()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()