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
                             QTabWidget, QListWidget, QListWidgetItem)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSize
from PyQt6.QtGui import QFont, QColor, QIcon, QPixmap


def resource_path(relative_path):
    """Получить абсолютный путь к ресурсу, работает для dev и для PyInstaller"""
    try:
        # PyInstaller создает временную папку и сохраняет путь в _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    
    return os.path.join(base_path, relative_path)


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
        """Создание страницы редактора проекта"""
        widget = QWidget()
        main_splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # Левая панель - Файловый менеджер
        left_panel = QFrame()
        left_panel.setFixedWidth(250)
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(5, 5, 5, 5)
        
        left_header = QLabel("📁 Файлы")
        left_header.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        left_layout.addWidget(left_header)
        
        self.file_tree = QTreeWidget()
        self.file_tree.setHeaderLabel("Проект")
        self.file_tree.itemDoubleClicked.connect(self.on_file_double_clicked)
        left_layout.addWidget(self.file_tree)
        
        # Центральная часть - вкладки
        center_tabs = QTabWidget()
        center_tabs.setDocumentMode(True)
        
        # Вкладка 2D Viewport
        viewport_tab = QWidget()
        viewport_layout = QVBoxLayout(viewport_tab)
        viewport_label = QLabel("🎮 2D Viewport")
        viewport_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        viewport_label.setFont(QFont("Segoe UI", 16))
        viewport_layout.addWidget(viewport_label)
        
        viewport_info = QLabel("Запустите проект для просмотра")
        viewport_info.setAlignment(Qt.AlignmentFlag.AlignCenter)
        viewport_layout.addWidget(viewport_info)
        
        center_tabs.addTab(viewport_tab, "2D")
        center_tabs.addTab(QWidget(), "3D")
        center_tabs.addTab(QWidget(), "Script")
        
        # Вкладка Inspector
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
        self.code_editor.setStyleSheet("""
            QTextEdit {
                background-color: #1e1e2e;
                color: #e0e0f0;
                border: none;
                padding: 10px;
            }
        """)
        self.code_editor.setPlainText("-- Редактор кода\n-- Выберите файл для редактирования\n\nfunction _ready()\n    print(\"Hello, BlazeBolt!\")\nend\n\nfunction _process(dt)\n    -- Обновление каждый кадр\nend")
        code_layout.addWidget(self.code_editor)
        
        center_tabs.addTab(code_tab, "Code")
        
        # Правая панель - Scene Tree
        right_panel = QFrame()
        right_panel.setFixedWidth(250)
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(5, 5, 5, 5)
        
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
        
        output_header = QLabel("📟 Output")
        output_header.setFont(QFont("Segoe UI", 10, QFont.Weight.Bold))
        output_layout.addWidget(output_header)
        
        self.output_list = QListWidget()
        self.output_list.addItem("[Info] Редактор готов")
        self.output_list.addItem("[System] Файловый менеджер инициализирован")
        output_layout.addWidget(self.output_list)
        
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
            # Открываем файл в редакторе
            ext = os.path.splitext(path)[1].lower()
            if ext in ['.lua', '.json', '.txt', '.md']:
                try:
                    with open(path, 'r', encoding='utf-8') as f:
                        content = f.read()
                    self.code_editor.setPlainText(content)
                    self.output_list.addItem(f"[File] Открыт: {path}")
                except Exception as e:
                    self.output_list.addItem(f"[Error] {str(e)}")
    
    def on_scene_item_clicked(self, item, column):
        """Обработчик клика по элементу сцены"""
        name = item.text(0)
        self.node_name_label.setText(f"Name: {name}")
        self.output_list.addItem(f"[Scene] Выбран: {name}")
    
    def open_editor(self, project_data):
        """Открыть редактор проекта"""
        self.current_project = project_data
        self.project_name_label.setText(project_data.get("name", "Проект"))
        
        # Загружаем файлы проекта
        self.load_project_files(project_data.get("path", ""))
        
        self.projects_widget.hide()
        self.editor_widget.show()
        self.output_list.addItem(f"[Project] Открыт: {project_data.get('name')}")
    
    def close_editor(self):
        """Закрыть редактор"""
        self.editor_widget.hide()
        self.projects_widget.show()
    
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
                    if os.path.isdir(item_path):
                        tree_item = QTreeWidgetItem([item])
                        parent.addChild(tree_item)
                        add_tree_items(tree_item, item_path)
                    else:
                        tree_item = QTreeWidgetItem([item])
                        tree_item.setData(0, 1, item_path)
                        parent.addChild(tree_item)
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
            """)
            self.theme_btn.setText("🌙")
        else:
            self.setStyleSheet("""
                QMainWindow, QWidget { background-color: #f5f5f5; color: #000000; }
                QPushButton {
                    background-color: #e0e0e0;
                    border: 1px solid #ccc;
                    border-radius: 8px;
                    padding: 8px;
                }
                QPushButton:hover { background-color: #d0d0d0; }
                QLineEdit {
                    background-color: white;
                    border: 1px solid #ccc;
                    border-radius: 8px;
                    padding: 8px;
                }
                QScrollArea { background-color: #f5f5f5; border: none; }
            """)
            self.theme_btn.setText("☀️")
            
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