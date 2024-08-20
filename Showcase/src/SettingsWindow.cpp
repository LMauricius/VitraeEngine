#include "SettingsWindow.h"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>

#include <mutex>

SettingsWindow::SettingsWindow(AssetCollection &assetCollection, Status &status)
    : QMainWindow(), ui(), m_assetCollection(assetCollection), m_status(status)
{
    ui.setupUi(this);

    updateValues();

    connect(ui.light_dir_x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.p_scene->light.direction.x = d;
    });
    connect(ui.light_dir_y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.p_scene->light.direction.y = d;
    });
    connect(ui.light_dir_z, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.p_scene->light.direction.z = d;
    });
    connect(ui.light_color, &QPushButton::clicked, [this]() {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        QColor lightColor = QColor::fromRgbF(m_assetCollection.p_scene->light.color_primary.r,
                                             m_assetCollection.p_scene->light.color_primary.g,
                                             m_assetCollection.p_scene->light.color_primary.b);
        QColor c = QColorDialog::getColor(lightColor, this);
        m_assetCollection.p_scene->light.color_primary = {c.redF(), c.greenF(), c.blueF()};
    });
    connect(ui.ambient_color, &QPushButton::clicked, [this]() {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        QColor lightColor = QColor::fromRgbF(m_assetCollection.p_scene->light.color_ambient.r,
                                             m_assetCollection.p_scene->light.color_ambient.g,
                                             m_assetCollection.p_scene->light.color_ambient.b);
        QColor c = QColorDialog::getColor(lightColor, this);
        m_assetCollection.p_scene->light.color_ambient = {c.redF(), c.greenF(), c.blueF()};
    });
    connect(ui.shadowMapSize, &QComboBox::currentTextChanged, [this](const QString &str) {
        this->m_assetCollection.comp.parameters.set("ShadowMapSize",
                                                    glm::vec2{str.toInt(), str.toInt()});
        this->m_assetCollection.shouldReloadPipelines = true;
    });
    m_assetCollection.comp.parameters.set("ShadowMapSize",
                                          glm::vec2{ui.shadowMapSize->currentText().toInt(),
                                                    ui.shadowMapSize->currentText().toInt()});

    connect(ui.ebrStrength, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d) {
        std::unique_lock lock1(this->m_assetCollection.accessMutex);
        this->m_assetCollection.comp.parameters.set("ebr_strength", (float)d);
    });

    // list methods
    for (auto &category : assetCollection.methodCategories) {
        auto combobox = new QComboBox(ui.shading_methods_group);

        for (auto &method : category.methods) {
            combobox->addItem(
                QString::fromStdString(std::string(method->p_composeMethod->getFriendlyName())));
        }

        combobox->setCurrentIndex(category.selectedIndex);

        connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, &category](int index) {
                    std::unique_lock lock1(this->m_assetCollection.accessMutex);
                    category.selectedIndex = index;
                    this->m_assetCollection.shouldReloadPipelines = true;
                });

        ui.shading_methods_layout->addRow(QString::fromStdString(category.name), combobox);
    }
}

SettingsWindow::~SettingsWindow() {}

void SettingsWindow::updateValues()
{
    ui.totalAvgDuraion->setText(QString::number(m_status.totalAvgFrameDuration.count() * 1000.0) +
                                "ms");
    ui.totalFPS->setText(QString::number(m_status.totalFPS));
    ui.currentAvgDuration->setText(
        QString::number(m_status.currentAvgFrameDuration.count() * 1000.0) + "ms");
    ui.currentFPS->setText(QString::number(m_status.currentFPS));

    // update spinboxes and other controls
    if (ui.camera_x->value() != m_assetCollection.p_scene->camera.position.x) {
        ui.camera_x->setValue(m_assetCollection.p_scene->camera.position.x);
    }
    if (ui.camera_y->value() != m_assetCollection.p_scene->camera.position.y) {
        ui.camera_y->setValue(m_assetCollection.p_scene->camera.position.y);
    }
    if (ui.camera_z->value() != m_assetCollection.p_scene->camera.position.z) {
        ui.camera_z->setValue(m_assetCollection.p_scene->camera.position.z);
    }
    auto cameraDirVector = m_assetCollection.p_scene->camera.rotation * glm::vec3(0, 0, 1);
    ui.camera_dir_x->setText(QString::number(cameraDirVector.x));
    ui.camera_dir_y->setText(QString::number(cameraDirVector.y));
    ui.camera_dir_z->setText(QString::number(cameraDirVector.z));
    if (ui.light_dir_x->value() != m_assetCollection.p_scene->light.direction.x) {
        ui.light_dir_x->setValue(m_assetCollection.p_scene->light.direction.x);
    }
    if (ui.light_dir_y->value() != m_assetCollection.p_scene->light.direction.y) {
        ui.light_dir_y->setValue(m_assetCollection.p_scene->light.direction.y);
    }
    if (ui.light_dir_z->value() != m_assetCollection.p_scene->light.direction.z) {
        ui.light_dir_z->setValue(m_assetCollection.p_scene->light.direction.z);
    }
    QColor lightColor(m_assetCollection.p_scene->light.color_primary.r * 255.0,
                      m_assetCollection.p_scene->light.color_primary.g * 255.0,
                      m_assetCollection.p_scene->light.color_primary.b * 255.0);
    ui.light_color->setPalette(
        QPalette(Qt::black, lightColor, lightColor, lightColor, lightColor, Qt::black, lightColor));
    lightColor = QColor(m_assetCollection.p_scene->light.color_ambient.r * 255.0,
                        m_assetCollection.p_scene->light.color_ambient.g * 255.0,
                        m_assetCollection.p_scene->light.color_ambient.b * 255.0);
    ui.ambient_color->setPalette(
        QPalette(Qt::black, lightColor, lightColor, lightColor, lightColor, Qt::black, lightColor));
}
