#include "smartweather.h"

#include <QComboBox>
#include <QCryptographicHash>
#include <QDate>
#include <QDateEdit>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPdfWriter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QStackedWidget>
#include <QTableView>
#include <QTextDocument>
#include <QTextStream>
#include <QUuid>
#include <QVBoxLayout>

enum Col { C_ID, C_NOM, C_PRENOM, C_EMAIL, C_POSTE, C_STRUCT, C_GOUV, C_DATE, C_STATUT, C_HORAIRE };

enum Page { PAGE_LOGIN = 0, PAGE_ACCUEIL = 1, PAGE_EMPLOYES = 2 };

static const QStringList ROLES_EMPLOYES = {"Responsable RH", "Chef de division", "Responsable régional"};

smartweather::smartweather(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Smart Weather Management");
    resize(1150, 700);
    if (!initDb()) return;

    m_stack = new QStackedWidget;
    m_stack->addWidget(buildLoginPage());
    m_stack->addWidget(buildAccueilPage());
    m_stack->addWidget(buildEmployesPage());

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_stack);

    appliquerStyle();
    m_stack->setCurrentIndex(PAGE_LOGIN);
}

bool smartweather::initDb()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("smartweather.db");
    if (!db.open()) {
        QMessageBox::critical(this, "Base de données", db.lastError().text());
        return false;
    }
    QSqlQuery q;
    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS employe ("
        "id_employe INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nom TEXT NOT NULL, prenom TEXT NOT NULL, email TEXT NOT NULL,"
        "poste TEXT, structure TEXT, gouvernorat TEXT,"
        "date_recrutement TEXT, statut TEXT, type_horaire TEXT)");
    ok = ok && q.exec(
             "CREATE TABLE IF NOT EXISTS utilisateur ("
             "id_utilisateur INTEGER PRIMARY KEY AUTOINCREMENT,"
             "login TEXT UNIQUE NOT NULL, mot_de_passe TEXT NOT NULL,"
             "sel TEXT NOT NULL, role TEXT NOT NULL)");
    if (ok) initUtilisateurs();
    return ok;
}

QString smartweather::hasher(const QString &motDePasse, const QString &sel)
{
    const QByteArray h = QCryptographicHash::hash((sel + motDePasse).toUtf8(),
                                                  QCryptographicHash::Sha256);
    return QString::fromLatin1(h.toHex());
}

void smartweather::initUtilisateurs()
{
    QSqlQuery c("SELECT COUNT(*) FROM utilisateur");
    if (c.next() && c.value(0).toInt() > 0) return;

    const struct { const char *login, *mdp, *role; } comptes[] = {
                    {"admin",     "admin123", "Responsable RH"},
                    {"prevision", "prev123",  "Prévisionniste"},
                    };
    for (const auto &u : comptes) {
        const QString sel = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QSqlQuery q;
        q.prepare("INSERT INTO utilisateur (login, mot_de_passe, sel, role) VALUES (?,?,?,?)");
        q.addBindValue(QString::fromUtf8(u.login));
        q.addBindValue(hasher(QString::fromUtf8(u.mdp), sel));
        q.addBindValue(sel);
        q.addBindValue(QString::fromUtf8(u.role));
        q.exec();
    }
}

QWidget *smartweather::buildLoginPage()
{
    auto *page = new QWidget;

    auto *logo = new QLabel("☁");
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size:48pt; color:#1E88E5;");
    auto *titre = new QLabel("Smart Weather Management");
    titre->setObjectName("titre");
    titre->setAlignment(Qt::AlignCenter);
    auto *sous = new QLabel("Institut National de la Météorologie");
    sous->setAlignment(Qt::AlignCenter);

    m_loginUser = new QLineEdit;
    m_loginUser->setPlaceholderText("Identifiant");
    m_loginPass = new QLineEdit;
    m_loginPass->setPlaceholderText("Mot de passe");
    m_loginPass->setEchoMode(QLineEdit::Password);
    m_loginErreur = new QLabel;
    m_loginErreur->setObjectName("erreur");
    m_loginErreur->setAlignment(Qt::AlignCenter);
    auto *btn = new QPushButton("Se connecter");

    auto *card = new QFrame;
    card->setObjectName("card");
    card->setFixedWidth(400);
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(30, 25, 30, 30);
    cl->setSpacing(12);
    cl->addWidget(logo);
    cl->addWidget(titre);
    cl->addWidget(sous);
    cl->addSpacing(10);
    cl->addWidget(m_loginUser);
    cl->addWidget(m_loginPass);
    cl->addWidget(m_loginErreur);
    cl->addWidget(btn);

    auto *h = new QHBoxLayout;
    h->addStretch();
    h->addWidget(card);
    h->addStretch();
    auto *v = new QVBoxLayout(page);
    v->addStretch();
    v->addLayout(h);
    v->addStretch();

    connect(btn, &QPushButton::clicked, this, &smartweather::seConnecter);
    connect(m_loginPass, &QLineEdit::returnPressed, this, &smartweather::seConnecter);
    connect(m_loginUser, &QLineEdit::returnPressed, m_loginPass, QOverload<>::of(&QWidget::setFocus));
    return page;
}

void smartweather::seConnecter()
{
    const QString user = m_loginUser->text().trimmed();
    const QString mdp = m_loginPass->text();

    QSqlQuery q;
    q.prepare("SELECT mot_de_passe, sel, role FROM utilisateur WHERE login=?");
    q.addBindValue(user);
    if (!q.exec() || !q.next() || hasher(mdp, q.value(1).toString()) != q.value(0).toString()) {
        m_loginErreur->setText("Identifiant ou mot de passe incorrect.");
        m_loginPass->clear();
        return;
    }

    m_user = user;
    m_role = q.value(2).toString();
    m_loginErreur->clear();
    m_loginUser->clear();
    m_loginPass->clear();

    m_bienvenue->setText(QString("Bienvenue, %1  •  %2").arg(m_user, m_role));
    const bool autorise = ROLES_EMPLOYES.contains(m_role);
    m_btnEmployes->setEnabled(autorise);
    m_btnEmployes->setToolTip(autorise ? QString() : "Accès réservé aux responsables RH, chefs de division et responsables régionaux.");

    m_stack->setCurrentIndex(PAGE_ACCUEIL);
}

void smartweather::deconnecter()
{
    m_user.clear();
    m_role.clear();
    m_stack->setCurrentIndex(PAGE_LOGIN);
    m_loginUser->setFocus();
}

QWidget *smartweather::buildAccueilPage()
{
    auto *page = new QWidget;

    auto *barre = new QFrame;
    barre->setObjectName("barre");
    auto *bl = new QHBoxLayout(barre);
    auto *appName = new QLabel("☁  Smart Weather Management");
    appName->setStyleSheet("font-size:14pt; font-weight:bold;");
    m_bienvenue = new QLabel;
    auto *btnDeco = new QPushButton("Déconnexion");
    btnDeco->setObjectName("danger");
    bl->addWidget(appName);
    bl->addStretch();
    bl->addWidget(m_bienvenue);
    bl->addWidget(btnDeco);

    auto *titre = new QLabel("Tableau de bord");
    titre->setObjectName("titre");
    auto *sous = new QLabel("Choisissez un module pour commencer.");

    m_btnEmployes = new QPushButton("Gestion des\nEmployés");
    auto *btnStations = new QPushButton("Gestion des\nStations");
    auto *btnReleves = new QPushButton("Relevés Météo\n& Attestations");
    auto *btnAlertes = new QPushButton("Alertes\n& Prédictions");
    for (auto *b : {m_btnEmployes, btnStations, btnReleves, btnAlertes})
        b->setObjectName("module");

    auto *grid = new QGridLayout;
    grid->setSpacing(20);
    grid->addWidget(m_btnEmployes, 0, 0);
    grid->addWidget(btnStations, 0, 1);
    grid->addWidget(btnReleves, 1, 0);
    grid->addWidget(btnAlertes, 1, 1);

    auto *corps = new QVBoxLayout;
    corps->setContentsMargins(60, 30, 60, 40);
    corps->addWidget(titre);
    corps->addWidget(sous);
    corps->addSpacing(15);
    corps->addLayout(grid, 1);

    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(barre);
    v->addLayout(corps, 1);

    connect(btnDeco, &QPushButton::clicked, this, &smartweather::deconnecter);
    connect(m_btnEmployes, &QPushButton::clicked, this, &smartweather::ouvrirEmployes);
    connect(btnStations, &QPushButton::clicked, this, &smartweather::moduleIndisponible);
    connect(btnReleves, &QPushButton::clicked, this, &smartweather::moduleIndisponible);
    connect(btnAlertes, &QPushButton::clicked, this, &smartweather::moduleIndisponible);
    return page;
}

void smartweather::moduleIndisponible()
{
    QMessageBox::information(this, "Module", "Ce module est en cours de développement par l'équipe.");
}

void smartweather::ouvrirEmployes()
{
    charger();
    m_stack->setCurrentIndex(PAGE_EMPLOYES);
}

void smartweather::retourAccueil()
{
    m_stack->setCurrentIndex(PAGE_ACCUEIL);
}

QWidget *smartweather::buildEmployesPage()
{
    auto *page = new QWidget;

    m_nom = new QLineEdit;
    m_prenom = new QLineEdit;
    m_email = new QLineEdit;
    m_gouvernorat = new QLineEdit;
    m_poste = new QComboBox;
    m_poste->addItems({"Ingénieur prévisionniste", "Agent d'observation",
                       "Technicien de maintenance", "Agent de service", "Administratif"});
    m_structure = new QComboBox;
    m_structure->addItems({"Siège", "Régionale"});
    m_statut = new QComboBox;
    m_statut->addItems({"Actif", "En congé", "Stagiaire"});
    m_horaire = new QComboBox;
    m_horaire->addItems({"Jour", "Garde 24h"});
    m_date = new QDateEdit(QDate::currentDate());
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("dd/MM/yyyy");

    auto *form = new QFormLayout;
    form->addRow("Nom :", m_nom);
    form->addRow("Prénom :", m_prenom);
    form->addRow("Email :", m_email);
    form->addRow("Poste :", m_poste);
    form->addRow("Structure :", m_structure);
    form->addRow("Gouvernorat :", m_gouvernorat);
    form->addRow("Date de recrutement :", m_date);
    form->addRow("Statut :", m_statut);
    form->addRow("Type d'horaire :", m_horaire);

    auto *btnAjouter = new QPushButton("Ajouter");
    auto *btnModifier = new QPushButton("Modifier");
    auto *btnSupprimer = new QPushButton("Supprimer");
    auto *btnVider = new QPushButton("Nouveau");
    btnSupprimer->setObjectName("danger");
    auto *crudLayout = new QHBoxLayout;
    crudLayout->addWidget(btnAjouter);
    crudLayout->addWidget(btnModifier);
    crudLayout->addWidget(btnSupprimer);
    crudLayout->addWidget(btnVider);

    auto *formBox = new QGroupBox("Fiche employé");
    auto *formLayout = new QVBoxLayout(formBox);
    formLayout->addLayout(form);
    formLayout->addLayout(crudLayout);
    formLayout->addStretch();

    auto *btnAccueil = new QPushButton("← Accueil");
    m_critere = new QComboBox;
    m_critere->addItems({"Nom", "Poste", "Structure", "Gouvernorat"});
    m_recherche = new QLineEdit;
    m_recherche->setPlaceholderText("Rechercher...");
    m_recherche->setMinimumWidth(180);
    m_tri = new QComboBox;
    m_tri->addItems({"Nom", "Poste", "Date de recrutement", "Structure"});

    auto *btnCsv = new QPushButton("Exporter CSV (Excel)");
    auto *btnPdf = new QPushButton("Exporter PDF");
    auto *btnStats = new QPushButton("Statistiques");

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(btnAccueil);
    toolbar->addWidget(new QLabel("Rechercher par :"));
    toolbar->addWidget(m_critere);
    toolbar->addWidget(m_recherche, 1);
    toolbar->addWidget(new QLabel("Trier par :"));
    toolbar->addWidget(m_tri);
    toolbar->addWidget(btnCsv);
    toolbar->addWidget(btnPdf);
    toolbar->addWidget(btnStats);

    m_model = new QSqlQueryModel(this);
    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->hide();

    auto *right = new QVBoxLayout;
    right->addLayout(toolbar);
    right->addWidget(m_table);

    auto *main = new QHBoxLayout(page);
    main->addWidget(formBox, 0);
    main->addLayout(right, 1);

    connect(btnAccueil, &QPushButton::clicked, this, &smartweather::retourAccueil);
    connect(btnAjouter, &QPushButton::clicked, this, &smartweather::ajouter);
    connect(btnModifier, &QPushButton::clicked, this, &smartweather::modifier);
    connect(btnSupprimer, &QPushButton::clicked, this, &smartweather::supprimer);
    connect(btnVider, &QPushButton::clicked, this, &smartweather::viderFormulaire);
    connect(btnCsv, &QPushButton::clicked, this, &smartweather::exporterCsv);
    connect(btnPdf, &QPushButton::clicked, this, &smartweather::exporterPdf);
    connect(btnStats, &QPushButton::clicked, this, &smartweather::afficherStats);
    connect(m_recherche, &QLineEdit::textChanged, this, &smartweather::charger);
    connect(m_critere, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &smartweather::charger);
    connect(m_tri, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &smartweather::charger);
    connect(m_table, &QTableView::clicked, this, &smartweather::remplirFormulaire);
    return page;
}

void smartweather::appliquerStyle()
{
    setStyleSheet(
        "QWidget { background:#F5F7FA; color:#37474F; font-family:Arial; font-size:12pt; }"
        "QLabel { background:transparent; }"
        "QLabel#titre { color:#1E88E5; font-size:22pt; font-weight:bold; }"
        "QLabel#erreur { color:#C62828; }"
        "QFrame#card { background:white; border:1px solid #B0BEC5; border-radius:10px; }"
        "QFrame#barre { background:#37474F; }"
        "QFrame#barre QLabel { color:white; }"
        "QGroupBox { font-weight:bold; border:1px solid #B0BEC5; border-radius:6px;"
        "           margin-top:12px; padding:10px; }"
        "QGroupBox::title { subcontrol-origin:margin; left:10px; color:#1E88E5; }"
        "QPushButton { background:#1E88E5; color:white; border:none;"
        "             border-radius:4px; padding:6px 12px; }"
        "QPushButton:hover { background:#1565C0; }"
        "QPushButton:disabled { background:#B0BEC5; color:#ECEFF1; }"
        "QPushButton#danger { background:#C62828; }"
        "QPushButton#module { font-size:16pt; font-weight:bold; border-radius:10px; min-height:110px; }"
        "QLineEdit, QComboBox, QDateEdit { background:white; border:1px solid #B0BEC5;"
        "                                  border-radius:4px; padding:6px; }"
        "QHeaderView::section { background:#1E88E5; color:white; padding:4px; border:none; }"
        "QTableView { background:white; alternate-background-color:#F5F7FA; }");
}

void smartweather::erreur(const QString &msg)
{
    QMessageBox::warning(this, "Erreur", msg);
}

bool smartweather::formulaireValide()
{
    if (m_nom->text().trimmed().isEmpty() || m_prenom->text().trimmed().isEmpty()) {
        erreur("Le nom et le prénom sont obligatoires.");
        return false;
    }
    static const QRegularExpression re(R"(^[\w.+-]+@[\w-]+\.[\w.-]+$)");
    if (!re.match(m_email->text().trimmed()).hasMatch()) {
        erreur("Adresse email invalide.");
        return false;
    }
    if (m_gouvernorat->text().trimmed().isEmpty()) {
        erreur("Le gouvernorat est obligatoire.");
        return false;
    }
    return true;
}

void smartweather::ajouter()
{
    if (!formulaireValide()) return;
    QSqlQuery q;
    q.prepare("INSERT INTO employe (nom,prenom,email,poste,structure,gouvernorat,"
              "date_recrutement,statut,type_horaire) VALUES (?,?,?,?,?,?,?,?,?)");
    q.addBindValue(m_nom->text().trimmed());
    q.addBindValue(m_prenom->text().trimmed());
    q.addBindValue(m_email->text().trimmed());
    q.addBindValue(m_poste->currentText());
    q.addBindValue(m_structure->currentText());
    q.addBindValue(m_gouvernorat->text().trimmed());
    q.addBindValue(m_date->date().toString("yyyy-MM-dd"));
    q.addBindValue(m_statut->currentText());
    q.addBindValue(m_horaire->currentText());
    if (!q.exec()) { erreur(q.lastError().text()); return; }
    viderFormulaire();
    charger();
}

void smartweather::modifier()
{
    if (m_idCourant < 0) { erreur("Sélectionnez un employé dans le tableau."); return; }
    if (!formulaireValide()) return;
    QSqlQuery q;
    q.prepare("UPDATE employe SET nom=?,prenom=?,email=?,poste=?,structure=?,gouvernorat=?,"
              "date_recrutement=?,statut=?,type_horaire=? WHERE id_employe=?");
    q.addBindValue(m_nom->text().trimmed());
    q.addBindValue(m_prenom->text().trimmed());
    q.addBindValue(m_email->text().trimmed());
    q.addBindValue(m_poste->currentText());
    q.addBindValue(m_structure->currentText());
    q.addBindValue(m_gouvernorat->text().trimmed());
    q.addBindValue(m_date->date().toString("yyyy-MM-dd"));
    q.addBindValue(m_statut->currentText());
    q.addBindValue(m_horaire->currentText());
    q.addBindValue(m_idCourant);
    if (!q.exec()) { erreur(q.lastError().text()); return; }
    charger();
}

void smartweather::supprimer()
{
    if (m_idCourant < 0) { erreur("Sélectionnez un employé dans le tableau."); return; }
    if (QMessageBox::question(this, "Confirmation",
                              "Supprimer définitivement cet employé ?") != QMessageBox::Yes) return;
    QSqlQuery q;
    q.prepare("DELETE FROM employe WHERE id_employe=?");
    q.addBindValue(m_idCourant);
    if (!q.exec()) { erreur(q.lastError().text()); return; }
    viderFormulaire();
    charger();
}

void smartweather::charger()
{
    static const char *colRecherche[] = {"nom", "poste", "structure", "gouvernorat"};
    static const char *colTri[] = {"nom", "poste", "date_recrutement", "structure"};

    QString sql = "SELECT id_employe, nom, prenom, email, poste, structure, gouvernorat, "
                  "date_recrutement, statut, type_horaire FROM employe";
    const QString texte = m_recherche->text().trimmed();
    if (!texte.isEmpty())
        sql += QString(" WHERE %1 LIKE ?").arg(colRecherche[m_critere->currentIndex()]);
    sql += QString(" ORDER BY %1").arg(colTri[m_tri->currentIndex()]);

    QSqlQuery q;
    q.prepare(sql);
    if (!texte.isEmpty()) q.addBindValue("%" + texte + "%");
    q.exec();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_model->setQuery(std::move(q));
#else
    m_model->setQuery(q);
#endif

    const QStringList titres = {"ID", "Nom", "Prénom", "Email", "Poste", "Structure",
                                "Gouvernorat", "Recrutement", "Statut", "Horaire"};
    for (int i = 0; i < titres.size(); ++i)
        m_model->setHeaderData(i, Qt::Horizontal, titres[i]);
}

void smartweather::remplirFormulaire(const QModelIndex &index)
{
    const QSqlRecord r = m_model->record(index.row());
    m_idCourant = r.value(C_ID).toInt();
    m_nom->setText(r.value(C_NOM).toString());
    m_prenom->setText(r.value(C_PRENOM).toString());
    m_email->setText(r.value(C_EMAIL).toString());
    m_poste->setCurrentText(r.value(C_POSTE).toString());
    m_structure->setCurrentText(r.value(C_STRUCT).toString());
    m_gouvernorat->setText(r.value(C_GOUV).toString());
    m_date->setDate(QDate::fromString(r.value(C_DATE).toString(), "yyyy-MM-dd"));
    m_statut->setCurrentText(r.value(C_STATUT).toString());
    m_horaire->setCurrentText(r.value(C_HORAIRE).toString());
}

void smartweather::viderFormulaire()
{
    m_idCourant = -1;
    m_nom->clear(); m_prenom->clear(); m_email->clear(); m_gouvernorat->clear();
    m_poste->setCurrentIndex(0); m_structure->setCurrentIndex(0);
    m_statut->setCurrentIndex(0); m_horaire->setCurrentIndex(0);
    m_date->setDate(QDate::currentDate());
    m_table->clearSelection();
}

void smartweather::exporterCsv()
{
    const QString path = QFileDialog::getSaveFileName(this, "Exporter", "employes.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) { erreur("Impossible d'écrire le fichier."); return; }
    QTextStream out(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out.setGenerateByteOrderMark(true);
    const int cols = m_model->columnCount();
    for (int c = 0; c < cols; ++c)
        out << m_model->headerData(c, Qt::Horizontal).toString() << (c + 1 < cols ? ";" : "\n");
    for (int r = 0; r < m_model->rowCount(); ++r)
        for (int c = 0; c < cols; ++c)
            out << m_model->data(m_model->index(r, c)).toString() << (c + 1 < cols ? ";" : "\n");
}

void smartweather::exporterPdf()
{
    const QString path = QFileDialog::getSaveFileName(this, "Exporter", "employes.pdf", "PDF (*.pdf)");
    if (path.isEmpty()) return;

    QString html = "<h2 style='color:#1E88E5'>Liste des employés - INM</h2>"
                   "<table border='1' cellspacing='0' cellpadding='4' width='100%'><tr>";
    const int cols = m_model->columnCount();
    for (int c = 0; c < cols; ++c)
        html += "<th bgcolor='#1E88E5'><font color='white'>" +
                m_model->headerData(c, Qt::Horizontal).toString() + "</font></th>";
    html += "</tr>";
    for (int r = 0; r < m_model->rowCount(); ++r) {
        html += "<tr>";
        for (int c = 0; c < cols; ++c)
            html += "<td>" + m_model->data(m_model->index(r, c)).toString().toHtmlEscaped() + "</td>";
        html += "</tr>";
    }
    html += "</table>";

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);
}

void smartweather::afficherStats()
{
    auto bloc = [](const QString &titre, const QString &col) {
        QString s = "<b>" + titre + "</b><br>";
        QSqlQuery q("SELECT " + col + ", COUNT(*) FROM employe GROUP BY " + col);
        while (q.next())
            s += q.value(0).toString() + " : " + q.value(1).toString() + "<br>";
        return s + "<br>";
    };
    QMessageBox::information(this, "Statistiques",
                             bloc("Par poste", "poste") + bloc("Par structure", "structure") +
                                 bloc("Par gouvernorat", "gouvernorat"));
}