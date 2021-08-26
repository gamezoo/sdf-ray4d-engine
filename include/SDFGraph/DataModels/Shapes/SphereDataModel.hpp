#pragma once

#include "SDFGraph/DataModels/BaseDataModel.hpp"

namespace sdfRay4d::sdfGraph
{
  class SphereDataModel : public BaseDataModel
  {
    Q_OBJECT

    public:
      SphereDataModel();
    virtual ~SphereDataModel() {}

    QString caption() const override { return QString("Sphere"); }
    QString name() const override { return QString("Sphere"); }

    QJsonObject save() const override { return {}; };
    void restore(QJsonObject const &p) override {};

    unsigned int nPorts(QtNodes::PortType portType) const override;
    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;
    void setInData(std::shared_ptr<QtNodes::NodeData>, int) override;

    QWidget *embeddedWidget() override;

    QString getData() override;

    private slots:
      void onTextEdited(QString const &_string);

    private:
      QLineEdit *m_scale;

      std::shared_ptr<MapData> m_data;

      sdfGraph::vec4 m_color;
      sdfGraph::vec4 m_dimensions;
  };
}