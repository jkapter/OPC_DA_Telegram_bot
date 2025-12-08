#include "copcclient.h"
//IOPCServerList uuid(13486D50-4821-11D2-A494-3CB306C10000)
__CRT_UUID_DECL(IOPCServerList, 0x13486D50, 0x4821, 0x11d2, 0xa4, 0x94, 0x3c, 0xb3, 0x06, 0xc1, 0x00, 0x00)

// IOPCServer uuid(39c13a4d-011e-11d0-9675-0020afd8adb3)
__CRT_UUID_DECL(IOPCServer, 0x39c13a4d, 0x011e, 0x11d0, 0x96, 0x75, 0x00, 0x20, 0xaf, 0xd8, 0xad, 0xb3)

// IOPCBrowseServerAddressSpace uuid(39c13a4f-011e-11d0-9675-0020afd8adb3)
__CRT_UUID_DECL(IOPCBrowseServerAddressSpace, 0x39c13a4f, 0x011e, 0x11d0, 0x96, 0x75, 0x00, 0x20, 0xaf, 0xd8, 0xad, 0xb3)

// IOPCItemMgt uuid(39c13a54-011e-11d0-9675-0020afd8adb3)
__CRT_UUID_DECL(IOPCItemMgt, 0x39c13a54, 0x011e, 0x11d0, 0x96, 0x75, 0x00, 0x20, 0xaf, 0xd8, 0xad, 0xb3)

// IOPCSyncIO uuid(39c13a52-011e-11d0-9675-0020afd8adb3)
__CRT_UUID_DECL(IOPCSyncIO, 0x39c13a52, 0x011e, 0x11d0, 0x96, 0x75, 0x00, 0x20, 0xaf, 0xd8, 0xad, 0xb3)

using namespace OPC_HELPER;

COPCClient::COPCClient() {
    qInfo() << QString("Новый экземпляр ОРС клиента, поток [%1]").arg(QThread::currentThread()->objectName());
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    get_registered_servers_();
}

COPCClient::~COPCClient() {
    clear_internal_data_();
    CoUninitialize();
    emit sg_send_message_to_console(QString("ОРС-клиент поток [%1]: завершен.").arg(QThread::currentThread()->objectName()));
    qInfo() << QString("Завершен ОРС клиент, поток %1").arg(QThread::currentThread()->objectName());
}

void COPCClient::clear_internal_data_() {
    for(const auto& [name, guid_ptr]: opc_server_name_to_guid_) {
        disconnect_server_(name);
    }
    guid_servers_.clear();
    opc_server_guid_to_data_.clear();
    opc_server_name_to_guid_.clear();
    opc_server_guid_to_tag_names_.clear();
    opc_server_guid_to_IOPCSErver_.clear();
    opc_server_guid_to_group_.clear();
}

void COPCClient::RefreshOPCServersList() {
    clear_internal_data_();
    get_registered_servers_();
}

int COPCClient::get_registered_servers_()
{
    CLSID clsid;
    CLSID clsidcat;
    HRESULT hRes;

    CLSIDFromString(L"{63D5F432-CFE4-11D1-B2C8-0060083BA1FB}", &clsidcat); //OPC DA 2.0 const class ID
    CLSIDFromProgID(L"OPC.ServerList", &clsid);

    IID IID_IOPCServerList = __uuidof(IOPCServerList);
    IOPCServerList2* pServerList = NULL;
    hRes = CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, IID_IOPCServerList, (void**)&pServerList);

    if(hRes != S_OK) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка запроса списка серверов. hRes = %2 : %3")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        if(pServerList) {
            pServerList->Release();
        }
        return -1;
    }

    if(!pServerList) {
        QString log_message = QString("ОРС-клиент поток [%1]: список серверов пуст.").arg(QThread::currentThread()->objectName());
        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;
        return -1;
    }

    IOPCEnumGUID* pIEnumGuid = NULL;
    hRes = pServerList->EnumClassesOfCategories(1, &clsidcat, 0, NULL, &pIEnumGuid);

    if(hRes != S_OK) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка запроса списка классов. hRes = %2 : %3")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        pServerList->Release();

        if(pIEnumGuid) {
            pServerList->Release();
        }
        return -1;
    }

    if(!pIEnumGuid) {
        QString log_message = QString("ОРС-клиент поток [%1]: список GUID серверов пуст.").arg(QThread::currentThread()->objectName());
        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;
        return -1;
    }

    wchar_t* pszProgID;
    wchar_t* pszUserType;
    wchar_t* pszVerIndProgID;

    GUID guid;
    int nServerCnt = 0;
    unsigned long iRetSvr;
    pIEnumGuid->Next(1, &guid, &iRetSvr);

    while (iRetSvr != 0)
    {
        nServerCnt++;
        pServerList->GetClassDetails(guid, &pszProgID, &pszUserType,  &pszVerIndProgID);
        guid_servers_.push_back(guid);
        opc_server_guid_to_data_[&guid_servers_.back()] = {QString::fromWCharArray(pszProgID), QString::fromWCharArray(pszUserType)};
        opc_server_name_to_guid_[QString::fromWCharArray(pszProgID)] = &guid_servers_.back();
        opc_server_guid_to_IOPCSErver_[&guid_servers_.back()] = NULL;
        opc_server_guid_to_group_[&guid_servers_.back()] = {};
        pIEnumGuid->Next(1, &guid, &iRetSvr);
    }

    pServerList->Release();
    pIEnumGuid->Release();
    return nServerCnt;
}

bool COPCClient::connect_and_browse_server_(const QString& server_name, std::vector<QString>& tags_names) {
    if(!connect_server_(server_name)) return false;

    //Подключение установлено
    IID IID_IOPCBrowseServerAddressSpace = __uuidof(IOPCBrowseServerAddressSpace);
    IOPCBrowseServerAddressSpace* pBrowse = NULL;

    HRESULT hRes = opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name))->QueryInterface(IID_IOPCBrowseServerAddressSpace, (void**)&pBrowse);

    if (hRes != S_OK) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка подключения к серверу. hRes = %2 : %3")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        if(pBrowse != NULL) {
            pBrowse->Release();
        }
        return false;
    }

    tags_names.clear();
    // отображаем содержимое сервера, начиная с корневого узла
    export_server_address_space_(pBrowse, tags_names);
    pBrowse->Release();

    disconnect_server_(server_name);

    return true;
}

void COPCClient::export_server_address_space_(IOPCBrowseServerAddressSpace* pParent, std::vector<QString>& tags_names)
{
    if(!pParent) return;

    IEnumString* pEnum = NULL;
    wchar_t* strName = NULL;
    wchar_t* lpItemID = NULL;

    unsigned long cnt;

    HRESULT hRes = pParent->BrowseOPCItemIDs(OPC_LEAF, L"", VT_EMPTY, 0, &pEnum);

    if(!pEnum) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка экспорта адресного пространства сервера OPC_LEAF. hRes = %2 : %3")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        return;
    }

    pEnum->Next(1, &strName, &cnt);

    while (cnt!=0) {
        pParent->GetItemID(strName, &lpItemID);//получает полный идентификатор тега
        tags_names.push_back(QString::fromWCharArray(lpItemID));
        pEnum->Next(1, &strName, &cnt);
    }
    pEnum->Release();
    pEnum = NULL;

    pParent->BrowseOPCItemIDs(OPC_BRANCH, L"", VT_EMPTY, 0, &pEnum);

    if(!pEnum) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка экспорта адресного пространства сервера OPC_BRANCH. hRes = %2 : %3")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        return;
    }

    pEnum->Next(1, &strName, &cnt);
    while (cnt != 0) {
        hRes = pParent->ChangeBrowsePosition(OPC_BROWSE_DOWN,strName);
        if (S_OK == hRes) {
            export_server_address_space_(pParent, tags_names);
            pParent->ChangeBrowsePosition(OPC_BROWSE_UP, strName);
            pEnum->Next(1, &strName, &cnt);
        } else {
            cnt = 0;
        }
    }
    pEnum->Release();
}

bool COPCClient::connect_server_(const QString& server_name) {
    if(opc_server_name_to_guid_.count(server_name) == 0) {
        QString log_message = QString("ОРС-клиент поток [%1]: не найден GUID сервера \"%2\" в списке.")
                                  .arg(QThread::currentThread()->objectName(), server_name);

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        return false;
    }

    if (opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)) != NULL) {
        return true;
    }

    HRESULT hRes;
    IID IID_IOPCSERVER = __uuidof(IOPCServer);
    GUID* guid_ptr = opc_server_name_to_guid_.at(server_name);

    hRes = CoCreateInstance(*guid_ptr, NULL, CLSCTX_LOCAL_SERVER, IID_IOPCSERVER, (void**)&opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)));
    if (hRes != S_OK) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка подключения к серверу %2. hRes = 0x%3 : %4")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(server_name)
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        if(opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)) != NULL) {
            opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name))->Release();
        }
        opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)) = NULL;
        return false;
    }

    QString log_message = QString("ОРС-клиент поток [%1]: подключениe к серверу %2 успешно")
                              .arg(QThread::currentThread()->objectName(), server_name);

    emit sg_send_message_to_console(log_message);
    qInfo() << log_message;

    return true;
}

bool COPCClient::register_group_(const QString& server_name)
{
    long bActive = 1;
    IOPCServer* server = opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name));
    OPCGroupHandler& hgroup = opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name));

    if(hgroup.opc_handle_group != 0 && hgroup.pItemMgt != NULL && hgroup.pSyncIO != NULL) return true;

    if(hgroup.opc_handle_group != 0 && (hgroup.pItemMgt || hgroup.pSyncIO)) {
        remove_group_(server_name);
    }

    HRESULT hRes = server->AddGroup(OLESTR("OPCDATG_GROUP"), bActive, hgroup.update_rate, 1, NULL, NULL, 0, &hgroup.opc_handle_group, &hgroup.update_rate, __uuidof(IOPCItemMgt), (IUnknown**)&hgroup.pItemMgt);

    if (FAILED(hRes)) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка добавления группы тэгов на сервер %2. hRes = 0х%3 : %4")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(server_name)
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        if(hgroup.pItemMgt != NULL) {
            hgroup.pItemMgt->Release();
        }
        hgroup.pItemMgt = NULL;
        hgroup.opc_handle_group = 0;

        return false;
    }

    IID IID_IOPCSYNCIO = __uuidof(IOPCSyncIO);
    hRes = hgroup.pItemMgt->QueryInterface(IID_IOPCSYNCIO, (void**)& hgroup.pSyncIO);

    if(FAILED(hRes)) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка запроса интерфейса сервера %2. hRes = 0х%3 : %4")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(server_name)
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        if(hgroup.pSyncIO != NULL) {
            hgroup.pSyncIO->Release();
        }
        return false;
    }
    return true;
}

void COPCClient::remove_group_(const QString& server_name)
{
    if(opc_server_name_to_guid_.count(server_name) == 0) return;
    if(opc_server_guid_to_IOPCSErver_.count(opc_server_name_to_guid_.at(server_name))
        || opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)) == NULL) return;

    OPCGroupHandler& group_handler = opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name));

    if(opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name)).pItemMgt != NULL) {
        if(opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name)).opc_handle_group != 0) {
            HRESULT* hErr = NULL;
            HRESULT hRes;
            if(group_handler.dwCount() > 0) {
                OPCHANDLE* item_handlers = group_handler.GetTagHandlesArrayToRead();
                hRes = opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name)).pItemMgt->RemoveItems(group_handler.dwCount(), item_handlers, &hErr);

                if(FAILED(hRes)) {
                    QString log_message = QString("ОРС-клиент поток [%1]: ошибка удаления элементов группы тэгов из сервера %2. hRes = 0х%3 : %4")
                                              .arg(QThread::currentThread()->objectName())
                                              .arg(server_name)
                                              .arg(static_cast<unsigned long>(hRes), 10, 16)
                                              .arg(GetErrorStringFromHRESULT(hRes));

                    emit sg_send_message_to_console(log_message);
                    qWarning() << log_message;
                }

                if(hErr != NULL) {
                    CoTaskMemFree(hErr);
                }
                delete[] item_handlers;
                group_handler.ClearTags();
            }
            if(group_handler.opc_handle_group != 0) {
                hRes = opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name))->RemoveGroup(group_handler.opc_handle_group, true);
                if(FAILED(hRes)) {
                    QString log_message = QString("ОРС-клиент поток [%1]: ошибка удаления группы тэгов из сервера %2. hRes = 0х%3 : %4")
                                              .arg(QThread::currentThread()->objectName())
                                              .arg(server_name)
                                              .arg(static_cast<unsigned long>(hRes), 10, 16)
                                              .arg(GetErrorStringFromHRESULT(hRes));

                    emit sg_send_message_to_console(log_message);
                    qWarning() << log_message;

                }
                group_handler.opc_handle_group = 0;
            }
        }
        group_handler.pItemMgt->Release();
        group_handler.pItemMgt = NULL;
    }
    if(opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name)).pSyncIO != NULL) {
        group_handler.pSyncIO->Release();
        group_handler.pSyncIO = NULL;
    }
}

void COPCClient::disconnect_server_(const QString& server_name)
{
    if(opc_server_name_to_guid_.count(server_name) == 0) {
        return;
    }

    remove_group_(server_name);

    if (opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)) != NULL) {
        opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name))->Release();
        opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)) = NULL;

        qInfo() << QString("OPC- клиент поток [%1]. Отключен от сервера %2")
                       .arg(QThread::currentThread()->objectName(), server_name);
    }
}

size_t COPCClient::read_server_tags_(const QString &server_name)
{
    if(     opc_server_name_to_guid_.count(server_name) == 0
        || opc_server_guid_to_group_.count(opc_server_name_to_guid_.at(server_name)) == 0) {
        return 0;
    }

    const OPCGroupHandler& group_hnd = opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name));

    if(group_hnd.dwCount() == 0) return 0;
    if(!connect_server_(server_name)) return 0;
    if(!register_group_(server_name)) return 0;

    OPCHANDLE* pHandles = group_hnd.GetTagHandlesArrayToRead();
    tagOPCITEMSTATE *pItemValue = NULL;
    HRESULT* pErrors;

    //Считываем элементы
    HRESULT hRes = group_hnd.pSyncIO->Read(OPC_DS_DEVICE, group_hnd.dwCount(), pHandles, &pItemValue, &pErrors);

    if(FAILED(hRes)) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка чтения тэгов из сервера %2. hRes = 0х%3 : %4")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(server_name)
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

    } else {
        for(size_t i = 0; i < group_hnd.dwCount(); ++i) {
            if(group_hnd.GetTagPtr(pHandles[i])) {
                group_hnd.GetTagPtr(pHandles[i])->SetOPCItemState(&pItemValue[i]);
            }
        }
    }

    CoTaskMemFree(pItemValue);
    CoTaskMemFree(pErrors);
    if(pHandles) delete[] pHandles;

    if(FAILED(hRes)) {
        return 0;
    } else {
        return group_hnd.dwCount();
    }
}

size_t COPCClient::write_server_tags_(const QString &server_name)
{
    if(opc_server_name_to_guid_.count(server_name) == 0 || opc_server_guid_to_group_.count(opc_server_name_to_guid_.at(server_name)) == 0) return 0;

    const OPCGroupHandler& group_hnd = opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name));

    if(group_hnd.dwCount() == 0) {

        return 0;
    }
    if(!connect_server_(server_name)) return 0;
    if(!register_group_(server_name)) return 0;

    auto [pHandles, pItemValues, dwCount] = group_hnd.GetTagHandlesArrayToWrite();

    if(dwCount == 0) {
        if(pHandles) delete[] pHandles;
        if(pItemValues) delete[] pItemValues;
        return 0;
    }

    HRESULT* pErrors;

    //Считываем элементы
    HRESULT hRes = group_hnd.pSyncIO->Write(dwCount, pHandles, pItemValues, &pErrors);

    if(FAILED(hRes)) {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка записи тэгов из сервера %2. hRes = 0х%3 : %4")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(server_name)
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

    } else {
        for(size_t i = 0; i < dwCount; ++i) {
            if(group_hnd.GetTagPtr(pHandles[i])) {
                group_hnd.GetTagPtr(pHandles[i])->ResetValueToWrite();
            }
        }
    }

    CoTaskMemFree(pErrors);

    if(pHandles) delete[] pHandles;
    if(pItemValues) delete[] pItemValues;

    if(FAILED(hRes)) {
        return 0;
    } else {
        return dwCount;
    }
}

size_t COPCClient::add_tags_to_group_(const QString& server_name, std::vector<std::shared_ptr<OPCTag>>& tags)
{
    if(opc_server_name_to_guid_.count(server_name) == 0) return 0;
    if(tags.size() == 0) return 0;
    if(!connect_server_(server_name) || !register_group_(server_name)) return 0;

    OPCGroupHandler& group_handler = opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(server_name));
    std::vector<std::shared_ptr<OPCTag>> tags_checked;

    for(const auto& it: tags) {
        if(!group_handler.CheckTagExist(it)) {
            tags_checked.push_back(it);
        }
    }

    DWORD count_tags = tags_checked.size();
    if(count_tags == 0) return 0;

    tagOPCITEMDEF* pItems = new tagOPCITEMDEF[count_tags];
    tagOPCITEMRESULT* pResults = NULL;
    HRESULT* pErrors = NULL;

    for(size_t i = 0; i < count_tags; ++i) {
        pItems[i] = tags_checked.at(i)->GetItemDefStruct();
    }

    HRESULT hRes = group_handler.pItemMgt->AddItems(count_tags, pItems, &pResults, &pErrors);

    if (!FAILED(hRes)) {
        for(size_t i = 0; i < count_tags; ++i) {
            group_handler.AddTagWithHandle(tags_checked.at(i), pResults[i].hServer);
        }
    } else {
        QString log_message = QString("ОРС-клиент поток [%1]: ошибка добавления тэгов в группу. hRes = 0х%2 : %3")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(static_cast<unsigned long>(hRes), 10, 16)
                                  .arg(GetErrorStringFromHRESULT(hRes));

        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;

        count_tags = 0;
    }
    CoTaskMemFree(pResults);
    CoTaskMemFree(pErrors);
    delete[] pItems;

    return count_tags;
}

std::set<QString> COPCClient::GetOPCServerNames() const {
    std::set<QString> ret_set;
    for(const auto& [guid, opc_data]: opc_server_guid_to_data_) {
        ret_set.insert(opc_data.ProgID);
    }
    return ret_set;
}

std::vector<QString> COPCClient::GetOPCTagsNames(const QString &server_name)
{
    if(opc_server_name_to_guid_.count(server_name) == 0) {
        return {};
    }

    if(opc_server_guid_to_tag_names_.count(opc_server_name_to_guid_.at(server_name)) > 0) {
        return opc_server_guid_to_tag_names_.at(opc_server_name_to_guid_.at(server_name));
    }

    std::vector<QString> ret_vec;
    if(!connect_and_browse_server_(server_name, ret_vec)) {
        return {};
    }

    opc_server_guid_to_tag_names_[opc_server_name_to_guid_.at(server_name)] = std::move(ret_vec);
    return opc_server_guid_to_tag_names_.at(opc_server_name_to_guid_.at(server_name));
}

size_t COPCClient::AddTags(std::vector<std::shared_ptr<OPCTag>> &tags)
{
    std::unordered_map<QString, std::vector<std::shared_ptr<OPCTag>>> tags_map_temp;
    size_t ret_val = 0;

    for(const auto& it: tags) {
        if(it && opc_server_name_to_guid_.count(it->GetServerName()) > 0
            && !opc_server_guid_to_group_.at(opc_server_name_to_guid_.at(it->GetServerName())).CheckTagExist(it)) {

            tags_map_temp[it->GetServerName()].push_back(it);
        }
    }

    for(auto& [name, opc_vec]: tags_map_temp) {
        ret_val += add_tags_to_group_(name, opc_vec);
    }

    return ret_val;
}

size_t COPCClient::ReadTags()
{
    size_t ret_val = 0;
    for(const auto& [guid, server_data]: opc_server_guid_to_data_) {
        ret_val += read_server_tags_(server_data.ProgID);
    }
    return ret_val;
}

size_t COPCClient::WriteTags()
{
    size_t ret_val = 0;
    for(const auto& [guid, server_data]: opc_server_guid_to_data_) {
        ret_val += write_server_tags_(server_data.ProgID);
    }
    return ret_val;
}

void COPCClient::ClearTags()
{
    for(auto& [guid_ptr, opc_group]: opc_server_guid_to_group_) {
        remove_group_(opc_server_guid_to_data_.at(guid_ptr).ProgID);
        opc_group.ClearTags();
    }
}

std::optional<OPCSERVERSTATUS> COPCClient::GetServerStatus(const QString &server_name)
{
    if(opc_server_name_to_guid_.count(server_name) == 0) return std::nullopt;
    if(opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name)) == NULL) {
        if(!connect_server_(server_name)) return std::nullopt;
    }

    OPCSERVERSTATUS* ret_str_ptr = NULL;
    OPCSERVERSTATUS ret_str;
    HRESULT hres = opc_server_guid_to_IOPCSErver_.at(opc_server_name_to_guid_.at(server_name))->GetStatus(&ret_str_ptr);
    if(FAILED(hres)) {
        CoTaskMemFree(ret_str_ptr);
        if(ret_str_ptr != NULL) {

        }
        return std::nullopt;
    }

    ret_str = *ret_str_ptr;
    CoTaskMemFree(ret_str_ptr);
    return ret_str;
}

QString OPC_HELPER::GetErrorStringFromHRESULT(HRESULT hr)
{
    QString retval;
    switch(hr)
    {
    case S_OK:					retval = "S_OK";break;
    case S_FALSE:				retval = "S_FALSE";break;
    case REGDB_E_WRITEREGDB:	retval = "REGDB_E_WRITEREGDB";break;
    case CO_E_CLASSSTRING:		retval = "CO_E_CLASSSTRING. The progID string was improperly formatted.";break;
    case E_NOINTERFACE:			retval = "E_NOINTERFACE";break;
    case E_FAIL:				retval = "E_FAIL";break;
    case E_OUTOFMEMORY:			retval = "E_OUTOFMEMORY";break;
    case E_INVALIDARG:			retval = "E_INVALIDARG";break;
    case E_NOTIMPL:				retval = "E_NOTIMPL";break;
    case OPC_E_INVALIDHANDLE:	retval = "OPC_E_INVALIDHANDLE";break;
    case OPC_E_UNKNOWNPATH:		retval = "OPC_E_UNKNOWNPATH";break;
    case OPC_E_UNKNOWNITEMID:	retval = "OPC_E_UNKNOWNITEMID";	break;
    case OPC_E_INVALIDITEMID:	retval = "OPC_E_INVALIDITEMID"; break;
    case OPC_S_UNSUPPORTEDRATE:	retval = "OPC_S_UNSUPPORTEDRATE";break;
    case OPC_E_PUBLIC:			retval = "OPC_E_PUBLIC";break;
    case OPC_E_INVALIDFILTER:	retval = "OPC_E_INVALIDFILTER";break;
    case DISP_E_TYPEMISMATCH:	retval = "DISP_E_TYPEMISMATCH";break;
    case OPC_E_BADTYPE:			retval = "OPC_E_BADTYPE";break;
    case OPC_S_CLAMP:			retval = "OPC_S_CLAMP";break;
    case OPC_E_RANGE:			retval = "OPC_E_RANGE";break;
    case OPC_E_BADRIGHTS:		retval = "OPC_E_BADRIGHTS";break;
    case DISP_E_OVERFLOW:		retval = "DISP_E_OVERFLOW";break;
    case OPC_E_DUPLICATENAME:	retval = "OPC_E_DUPLICATENAME";break;
    case static_cast<HRESULT>(0x800706ba):			retval = "RPC server unavailable";break;
    case static_cast<HRESULT>(0x80040154):			retval = "Class not registered.";break;
    case RPC_S_SERVER_UNAVAILABLE: retval = "RPC_S_SERVER_UNAVAILABLE";break;
    default:					retval = QString("Errorcode 0x%1 not defined.").arg(static_cast<unsigned long>(hr), 10, 16);
    }
    return retval;
}

//=======================================================
//===== OPCGroupHandler =================================
//=======================================================

DWORD OPCGroupHandler::dwCount() const
{
    return static_cast<unsigned long>(tag_to_opchandle_.size());
}

[[nodiscard]] std::tuple<OPCHANDLE*, VARIANT*, DWORD> OPCGroupHandler::GetTagHandlesArrayToWrite() const
{
    OPCHANDLE* ret_ptr = nullptr;
    VARIANT* ret_values = nullptr;
    DWORD ret_cnt = 0;

    if(tag_to_opchandle_.size() > 0) {
        ret_ptr = new OPCHANDLE[tag_to_opchandle_.size()];
        ret_values = new VARIANT[tag_to_opchandle_.size()];

        for(const auto& [tag_ptr, hnd]: tag_to_opchandle_) {
            if(tag_ptr->GetOPCVariantToWrite().has_value()) {
                ret_ptr[ret_cnt] = hnd;
                ret_values[ret_cnt++] = tag_ptr->GetOPCVariantToWrite().value();
            }
        }
    }
    return {ret_ptr, ret_values, ret_cnt};
}

[[nodiscard]] OPCHANDLE* OPCGroupHandler::GetTagHandlesArrayToRead() const
{
    OPCHANDLE* ret_ptr = nullptr;
    if(tag_to_opchandle_.size() > 0) {
        ret_ptr = new OPCHANDLE[tag_to_opchandle_.size()];
        size_t i = 0;
        for(const auto& [tag_ptr, hnd]: tag_to_opchandle_) {
            ret_ptr[i++] = hnd;
        }
    }
    return ret_ptr;
}

bool OPCGroupHandler::AddTagWithHandle(std::shared_ptr<OPCTag> &tag, OPCHANDLE hnd)
{
    if(!CheckTagExist(tag)) {
        tag_to_opchandle_[tag] = hnd;
        opchandle_to_tag_[hnd] = tag;
        return true;
    }
    return false;
}

void OPCGroupHandler::ClearTags()
{
    tag_to_opchandle_.clear();
    opchandle_to_tag_.clear();
}

std::shared_ptr<OPCTag> OPCGroupHandler::GetTagPtr(OPCHANDLE hnd) const
{
    if(opchandle_to_tag_.count(hnd) > 0) return opchandle_to_tag_.at(hnd);
    return nullptr;
}

OPCHANDLE OPCGroupHandler::GetTagHandle(std::shared_ptr<OPCTag> &tag) const
{
    if(tag_to_opchandle_.count(tag) > 0) return tag_to_opchandle_.at(tag);
    return 0;
}

size_t OPCGroupHandler::GetTagsCount() const
{
    return opchandle_to_tag_.size();
}

bool OPCGroupHandler::CheckTagExist(const std::shared_ptr<OPCTag>& tag) const
{
    return tag_to_opchandle_.count(tag) > 0;
}
