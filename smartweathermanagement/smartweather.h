#ifndef SMARTWEATHER_H
#define SMARTWEATHER_H

#include <QWidget>

class QLineEdit;
class QComboBox;
class QDateEdit;
class QTableView;
class QSqlQueryModel;


class smartweather : public QWidget
{
    Q_OBJECT
public:
    explicit smartweather(QWidget *parent = nullptr);

private slots:
    void ajouter();
    void modifier();
    void supprimer();
    void charger();
    void exporterCsv();
    void exporterPdf();
    void afficherStats();
    void remplirFormulaire(const QModelIndex &index);
    void viderFormulaire();

private:
    void buildUi();
    bool initDb();
    bool formulaireValide();
    void erreur(const QString &msg);

    // Formulaire
    QLineEdit *m_nom, *m_prenom, *m_email, *m_gouvernorat;
    QComboBox *m_poste, *m_structure, *m_statut, *m_horaire;
    QDateEdit *m_date;
    // Recherche / tri
    QComboBox *m_critere, *m_tri;
    QLineEdit *m_recherche;
    // Tableau
    QTableView *m_table;
    QSqlQueryModel *m_model;
    int m_idCourant = -1;
};

#endif