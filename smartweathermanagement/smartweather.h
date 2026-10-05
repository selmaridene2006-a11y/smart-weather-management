#ifndef SMARTWEATHER_H
#define SMARTWEATHER_H

#include <QWidget>

class QLineEdit;
class QComboBox;
class QDateEdit;
class QTableView;
class QSqlQueryModel;
class QStackedWidget;
class QLabel;
class QPushButton;

class smartweather : public QWidget
{
    Q_OBJECT
public:
    explicit smartweather(QWidget *parent = nullptr);

private slots:
    void seConnecter();
    void deconnecter();
    void ouvrirEmployes();
    void retourAccueil();
    void moduleIndisponible();

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
    bool initDb();
    void initUtilisateurs();
    static QString hasher(const QString &motDePasse, const QString &sel);
    void appliquerStyle();

    QWidget *buildLoginPage();
    QWidget *buildAccueilPage();
    QWidget *buildEmployesPage();

    bool formulaireValide();
    void erreur(const QString &msg);

    QStackedWidget *m_stack;
    QLineEdit *m_loginUser, *m_loginPass;
    QLabel *m_loginErreur, *m_bienvenue;
    QPushButton *m_btnEmployes;
    QString m_user, m_role;

    QLineEdit *m_nom, *m_prenom, *m_email, *m_gouvernorat;
    QComboBox *m_poste, *m_structure, *m_statut, *m_horaire;
    QDateEdit *m_date;
    QComboBox *m_critere, *m_tri;
    QLineEdit *m_recherche;
    QTableView *m_table;
    QSqlQueryModel *m_model;
    int m_idCourant = -1;
};

#endif